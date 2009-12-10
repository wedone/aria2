/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2009 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "UTMetadataDataExtensionMessage.h"
#include "BDE.h"
#include "bencode.h"
#include "util.h"
#include "a2functional.h"
#include "DownloadContext.h"
#include "UTMetadataRequestTracker.h"
#include "PieceStorage.h"
#include "BtConstants.h"
#include "MessageDigestHelper.h"
#include "bittorrent_helper.h"
#include "DiskAdaptor.h"
#include "Piece.h"
#include "LogFactory.h"

namespace aria2 {

UTMetadataDataExtensionMessage::UTMetadataDataExtensionMessage
(uint8_t extensionMessageID):UTMetadataExtensionMessage(extensionMessageID),
			     _logger(LogFactory::getInstance()) {}

std::string UTMetadataDataExtensionMessage::getPayload()
{
  BDE dict = BDE::dict();
  dict["msg_type"] = 1;
  dict["piece"] = _index;
  dict["total_size"] = _totalSize;
  return bencode::encode(dict)+_data;
}

std::string UTMetadataDataExtensionMessage::toString() const
{
  return strconcat("ut_metadata data piece=", util::uitos(_index));
}

void UTMetadataDataExtensionMessage::doReceivedAction()
{
  if(_tracker->tracks(_index)) {
    _logger->debug("ut_metadata index=%lu found in tracking list",
		  static_cast<unsigned long>(_index));
    _tracker->remove(_index);
    _pieceStorage->getDiskAdaptor()->writeData
      (reinterpret_cast<const unsigned char*>(_data.c_str()), _data.size(),
       _index*METADATA_PIECE_SIZE);
    _pieceStorage->completePiece(_pieceStorage->getPiece(_index));
    if(_pieceStorage->downloadFinished()) {
      std::string metadata = util::toString(_pieceStorage->getDiskAdaptor());
      unsigned char infoHash[INFO_HASH_LENGTH];
      MessageDigestHelper::digest(infoHash, INFO_HASH_LENGTH,
				  MessageDigestContext::SHA1,
				  metadata.data(), metadata.size());
      const BDE& attrs = _dctx->getAttribute(bittorrent::BITTORRENT);
      if(std::string(&infoHash[0], &infoHash[INFO_HASH_LENGTH]) == 
	 attrs[bittorrent::INFO_HASH].s()){
	_logger->info("Got ut_metadata");
      } else {
	_logger->info("Got wrong ut_metadata");
	for(size_t i = 0; i < _dctx->getNumPieces(); ++i) {
	  _pieceStorage->markPieceMissing(i);
	}
      }
    }
  } else {
    _logger->debug("ut_metadata index=%lu is not tracked",
		  static_cast<unsigned long>(_index));
  }
}

} // namespace aria2