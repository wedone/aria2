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
#include "CreateRequestCommand.h"

#include "InitiateConnectionCommandFactory.h"
#include "RequestGroup.h"
#include "Segment.h"
#include "DownloadContext.h"
#include "DlAbortEx.h"
#include "DownloadEngine.h"
#include "SocketCore.h"
#include "SegmentMan.h"

namespace aria2 {

CreateRequestCommand::CreateRequestCommand(int32_t cuid,
					   RequestGroup* requestGroup,
					   DownloadEngine* e):
  AbstractCommand
  (cuid, SharedHandle<Request>(), SharedHandle<FileEntry>(), requestGroup, e)
{
  setStatus(Command::STATUS_ONESHOT_REALTIME);
  disableReadCheckSocket();
  disableWriteCheckSocket();
}
		  
bool CreateRequestCommand::executeInternal()
{
  if(_segments.empty()) {
    _fileEntry = _requestGroup->getDownloadContext()->findFileEntryByOffset(0);
  } else {
    // We assume all segments belongs to same file.
    _fileEntry = _requestGroup->getDownloadContext()->findFileEntryByOffset
      (_segments.front()->getPositionToWrite());
  }
  req = _fileEntry->getRequest(_requestGroup->getURISelector());
  if(req.isNull()) {
    if(!_requestGroup->getSegmentMan().isNull()) {
      _requestGroup->getSegmentMan()->ignoreSegmentFor(_fileEntry);
    }
    throw DL_ABORT_EX("No URI available.");
  }

  Command* command =
    InitiateConnectionCommandFactory::createInitiateConnectionCommand
    (cuid, req, _fileEntry, _requestGroup, e);
  e->setNoWait(true);
  e->commands.push_back(command);
  return true;
}

} // namespace aria2