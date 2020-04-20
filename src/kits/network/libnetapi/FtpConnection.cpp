/*
 * Copyright 2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Khaled Emara, mail@KhaledEmara.dev
 */


#include <FtpConnection.h>

#include <arpa/inet.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>

#include <AutoDeleter.h>
#include <BufferedDataIO.h>
#include <DataIO.h>
#include <Debug.h>
#include <File.h>
#include <Socket.h>


BFtpResponse::BFtpResponse(int16 code, const BString& message)
	:
	fStatus(code),
	fMessage(message)
{
}



BFtpResponse::BFtpResponse(const BFtpResponse& other)
    :
    fStatus(other.GetStatus()),
	fMessage(other.GetMessage())
{ 
}


bool
BFtpResponse::IsOk() const
{
    return fStatus < B_FTP_STATUS__TRANSIENT_NEGATIVE_COMPLETION_BASE;
}



int16
BFtpResponse::GetStatus() const
{
    return fStatus;
}



const BString&
BFtpResponse::GetMessage() const
{
    return fMessage;
}



BFtpDirectoryResponse::BFtpDirectoryResponse(const BFtpResponse& response)
	:
	BFtpResponse(response)
{
    if (IsOk()) {
        int32 begin = GetMessage().FindFirst('"', 0);
        int32 end   = GetMessage().FindFirst('"', begin + 1);
        GetMessage().CopyInto(fDirectory, begin + 1, end - begin - 1);
    }
}



const BString&
BFtpDirectoryResponse::GetDirectory() const
{
    return fDirectory;
}



BFtpListingResponse::BFtpListingResponse(const BFtpResponse& response,
	const BString& data)
	:
	BFtpResponse(response)
{
    if (IsOk()) {
        int32 lastPos = 0;
        BString placeholder;
        for (int32 pos = data.FindFirst("\r\n");
			 pos != B_ERROR;
			 pos = data.FindFirst("\r\n", lastPos)) {
                data.CopyInto(placeholder, lastPos, pos - lastPos);
			    fListing.push_back(placeholder);
			    lastPos = pos + 2;
        }
    }
}



const std::vector<BString>&
BFtpListingResponse::GetListing() const
{
    return fListing;
}



BFtpConnection::BFtpConnection(const BUrl& url, const char* protocolName,
	BUrlProtocolListener* listener, BUrlContext* context)
	:
	BNetworkRequest(url, listener, context, "BUrlProtocol.FTP", protocolName),
	fCommand(kCommandInvalid),
    fResult(B_FTP_STATUS_INVALID_RESPONSE),
	fMode(B_FTP_TRANSFER_MODE_BINARY),
	fUsername("anonymous"),
	fPassword("haiku-development@freelists.org"),
	fRemotePath(""),
	fNewRemotePath(""),
	fLocalPath(""),
	fLogin(false),
	fKeepAlive(false),
	fAppend(false)
{
	fSocket = NULL;
}



BFtpConnection::BFtpConnection(const BFtpConnection& other)
	:
	BNetworkRequest(other.Url(), other.fListener, other.fContext,
		"BUrlProtocol.FTP", "FTP"),
	fCommand(kCommandInvalid),
    fResult(B_FTP_STATUS_INVALID_RESPONSE),
	fMode(B_FTP_TRANSFER_MODE_BINARY),
	fUsername("anonymous"),
	fPassword("haiku-development@freelists.org"),
	fRemotePath(""),
	fNewRemotePath(""),
	fLocalPath(""),
	fLogin(false),
	fKeepAlive(false),
	fAppend(false)
{
	fSocket = NULL;
}



BFtpConnection::~BFtpConnection()
{
    Stop();

	delete fSocket;
}



void
BFtpConnection::Login()
{
    return Login("anonymous", "haiku-development@freelists.org");
}



void
BFtpConnection::Login(const BString& name, const BString& password)
{
	fUsername = name;
	fPassword = password;
	fLogin = true;
}



BFtpResponse
BFtpConnection::_Login()
{
    BFtpResponse response = SendCommand("USER", fUsername);
    if (response.IsOk())
        response = SendCommand("PASS", fPassword);

    return response;
}



void
BFtpConnection::KeepAlive()
{
    fKeepAlive = true;
}



BFtpResponse
BFtpConnection::_KeepAlive()
{
    return SendCommand("NOOP");
}



void
BFtpConnection::GetWorkingDirectory()
{
	fCommand = kCommandGetWorkingDirectory;
}



BFtpDirectoryResponse
BFtpConnection::_GetWorkingDirectory()
{
    return BFtpDirectoryResponse(SendCommand("PWD"));
}



void
BFtpConnection::GetDirectoryListing(const BString& directory)
{
    fRemotePath = directory;
	fCommand = kCommandGetDirectoryListing;
}



BFtpListingResponse
BFtpConnection::_GetDirectoryListing()
{
    std::ostringstream directoryData;
	char buffer[4096];
	BDataIO inputStream;
	BBufferedDataIO inputBuffer(inputStream);
    BFtpDataChannel data(*this);
	size_t received;

    BFtpResponse response = data.Open(B_FTP_TRANSFER_MODE_TEXT);
    if (response.IsOk()) {
        response = SendCommand("NLST", fRemotePath);
        if (response.IsOk()) {
            data.Receive(inputStream);
            response = _GetResponse();
        }
    }

	while ((received = inputBuffer.Read(buffer, 4096)) > 0) {
		directoryData << buffer;
	}

    return BFtpListingResponse(response, BString(directoryData.str().c_str()));
}



void
BFtpConnection::ChangeDirectory(const BString& directory)
{
    fRemotePath = directory;
	fCommand = kCommandChangeDirectory;
}



BFtpResponse
BFtpConnection::_ChangeDirectory()
{
    return SendCommand("CWD", fRemotePath);
}



void
BFtpConnection::ParentDirectory()
{
    fCommand = kCommandParentDirectory;
}



BFtpResponse
BFtpConnection::_ParentDirectory()
{
    return SendCommand("CDUP");
}



void
BFtpConnection::CreateDirectory(const BString& name)
{
    fRemotePath = name;
	fCommand = kCommandCreateDirectory;
}



BFtpResponse
BFtpConnection::_CreateDirectory()
{
    return SendCommand("MKD", fRemotePath);
}



void
BFtpConnection::DeleteDirectory(const BString& name)
{
    fRemotePath = name;
	fCommand = kCommandDeleteDirectory;
}



BFtpResponse
BFtpConnection::_DeleteDirectory()
{
    return SendCommand("RMD", fRemotePath);
}



void
BFtpConnection::RenameFile(const BString& file, const BString& newName)
{
    fRemotePath = file;
	fNewRemotePath = newName;
	fCommand = kCommandRenameFile;
}



BFtpResponse
BFtpConnection::_RenameFile()
{
    BFtpResponse response = SendCommand("RNFR", fRemotePath);
    if (response.IsOk())
       response = SendCommand("RNTO", fNewRemotePath);

    return response;
}



void
BFtpConnection::DeleteFile(const BString& name)
{
    fRemotePath = name;
	fCommand = kCommandDeleteFile;
}



BFtpResponse
BFtpConnection::_DeleteFile()
{
    return SendCommand("DELE", fRemotePath);
}


void
BFtpConnection::Download(const BString& remoteFile, const BString& localPath,
	int8 mode)
{
    fRemotePath = remoteFile;
	fLocalPath = localPath;
	fMode = mode;
	fCommand = kCommandDownload;
}



BFtpResponse
BFtpConnection::_Download()
{
    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(fMode);
    if (response.IsOk()) {
        response = SendCommand("RETR", fRemotePath);
        if (response.IsOk()) {
            BString filename = fRemotePath;
            int32 pos = filename.FindLast('\\');
            pos = std::max(pos, filename.FindLast('/'));
            if (pos != B_ERROR)
                filename.CopyInto(filename, pos + 1, filename.Length() - pos - 1);

            BString path = fLocalPath;
            if (!path.IsEmpty() && (path[path.Length() - 1] != '\\') &&
                (path[path.Length() - 1] != '/'))
                    path += "/";

            BFile file(path.Append(filename).String(),
                            	  B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

            data.Receive(file);

            response = _GetResponse();

            if (!response.IsOk())
                std::remove((path).String());
        }
    }

    return response;
}



void
BFtpConnection::Upload(const BString& localFile, const BString& remotePath,
	int8 mode, bool append)
{
	fLocalPath = localFile;
    fRemotePath = remotePath;
	fMode = mode;
	fAppend = append;
	fCommand = kCommandUpload;
}



BFtpResponse
BFtpConnection::_Upload()
{
    std::ifstream file(fLocalPath.String(), std::ios_base::binary);
    if (!file)
        return BFtpResponse(B_FTP_STATUS_INVALID_FILE);

    BString filename = fLocalPath;
    int32 pos = filename.FindLast('\\');
    pos = std::max(pos, filename.FindLast('/'));
    if (pos != B_ERROR)
        filename.CopyInto(filename, pos + 1, filename.Length() - pos - 1);

    BString path = fRemotePath;
    if (!path.IsEmpty() && (path[path.Length() - 1] != '\\') &&
        (path[path.Length() - 1] != '/'))
            path += "/";

    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(fMode);
    if (response.IsOk()) {
        response = SendCommand(fAppend ? "APPE" : "STOR", path += filename);
        if (response.IsOk()) {
            data.Send(file);

            response = _GetResponse();
        }
    }

    return response;
}



BFtpResponse
BFtpConnection::SendCommand(const BString& command, const BString& parameter)
{
    BString commandStr;
    if (parameter != "")
        commandStr.Append(command).Append(" ").Append(parameter).Append("\r\n");
    else
        commandStr.Append(command).Append("\r\n");

    fSocket->Write(commandStr.String(), commandStr.Length());

    return _GetResponse();
}



status_t
BFtpConnection::Connect()
{
	delete fSocket;

	fSocket = new(std::nothrow) BSocket();

	if (fSocket == NULL)
		return B_NO_MEMORY;

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Connection to %s on port %d.",
		fUrl.Authority().String(), fRemoteAddr.Port());
	return fSocket->Connect(fRemoteAddr);
}



status_t
BFtpConnection::Stop()
{
	BFtpResponse response = SendCommand("QUIT");
    if (response.IsOk() && fSocket != NULL)
		fSocket->Disconnect();

	return BNetworkRequest::Stop();
}



const BFtpResponse&
BFtpConnection::ResultantResponse() const
{
	return fResult;
}



status_t
BFtpConnection::_ProtocolLoop()
{
	BString host = fUrl.Host();
	int port = 21;

	if (fUrl.HasPort())
		port = fUrl.Port();

	if (fContext->UseProxy()) {
		host = fContext->GetProxyHost();
		port = fContext->GetProxyPort();
	}

	if (!_ResolveHostName(host, port)) {
		_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR,
			"Unable to resolve hostname (%s), aborting.",
				fUrl.Host().String());
		return B_SERVER_NOT_FOUND;
	}

	status_t connectError = Connect();
	if (connectError != B_OK) {
		_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "Socket connection error %s",
			strerror(connectError));
		return connectError;
	}

	BFtpResponse response = _GetResponse();
	if (!response.IsOk())
		return B_RESOURCE_UNAVAILABLE;

	if (fListener != NULL)
		fListener->ConnectionOpened(this);

	_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT,
		"Connection opened, sending request.");

    fResult = _Login();

    if (!fResult.IsOk())
		return B_RESOURCE_UNAVAILABLE;

	switch (fCommand) {
		case kCommandInvalid:
			return B_RESOURCE_NOT_FOUND;
		case kCommandGetWorkingDirectory:
			fResult = _GetWorkingDirectory();
			break;
		case kCommandGetDirectoryListing:
			fResult = _GetDirectoryListing();
			break;
		case kCommandChangeDirectory:
			fResult = _ChangeDirectory();
			break;
		case kCommandParentDirectory:
			fResult = _ParentDirectory();
			break;
		case kCommandCreateDirectory:
			fResult = _CreateDirectory();
			break;
		case kCommandDeleteDirectory:
			fResult = _DeleteDirectory();
			break;
		case kCommandRenameFile:
			fResult = _RenameFile();
			break;
		case kCommandDeleteFile:
			fResult = _DeleteFile();
			break;
		case kCommandDownload:
			fResult = _Download();
			break;
		case kCommandUpload:
			fResult = _Upload();
			break;
    }

	if (!fResult.IsOk())
		return B_RESOURCE_UNAVAILABLE;

	return B_OK;
}



BFtpResponse
BFtpConnection::_GetResponse()
{
    unsigned int lastCode  = 0;
    bool isInsideMultiline = false;
    std::string message;

    while (true) {
        char buffer[1024];
        std::size_t length, bytesReceived = 0;

        if (fReceiveBuffer.IsEmpty()) {
            length = fSocket->Read(buffer, sizeof(buffer));
            bytesReceived += length;
            if (fListener != NULL) {
                fListener->DataReceived(this, buffer,
                                bytesReceived - length, length);
            }
        } else {
            fReceiveBuffer.CopyInto(buffer, 0, fReceiveBuffer.Length());
            length = fReceiveBuffer.Length();
            fReceiveBuffer.SetTo("");
        }

        std::istringstream in(std::string(buffer, length),
                              std::ios_base::binary);
        while (in) {
            unsigned int code;
            if (in >> code) {
                char separator;
                in.get(separator);

                if ((separator == '-') && !isInsideMultiline) {
                    isInsideMultiline = true;

                    if (lastCode == 0)
                        lastCode = code;

                    std::getline(in, message);

                    message.erase(message.length() - 1);
                    message = separator + message + "\n";
                } else {
                    if ((separator != '-') &&
                        ((code == lastCode) || (lastCode == 0))) {
                        std::string line;
                        std::getline(in, line);

                        line.erase(line.length() - 1);

                        if (code == lastCode) {
                            std::ostringstream out;
                            out << code << separator << line;
                            message += out.str();
                        } else {
                            message = separator + line;
                        }

                        fReceiveBuffer.SetTo(buffer +
                            static_cast<std::size_t>(in.tellg()),
                            length - static_cast<std::size_t>(in.tellg()));

                        return BFtpResponse(code, BString(message.c_str()));
                    } else {
                        std::string line;
                        std::getline(in, line);

                        if (!line.empty()) {
                            line.erase(line.length() - 1);

                            std::ostringstream out;
                            out << code << separator << line << "\n";
                            message += out.str();
                        }
                    }
                }
            } else if (lastCode != 0) {
                in.clear();

                std::string line;
                std::getline(in, line);

                if (!line.empty()) {
                    line.erase(line.length() - 1);

                    message += line + "\n";
                }
            } else {
                return BFtpResponse(B_FTP_STATUS_INVALID_RESPONSE);
            }
        }
    }
}



BFtpDataChannel::BFtpDataChannel(BFtpConnection& owner)
	:
	fFtpConnection(owner)
{
}



BFtpResponse
BFtpDataChannel::Open(int8 mode)
{
    // Open a data connection in active mode (we connect to the server)
    BFtpResponse response = fFtpConnection.SendCommand("PASV");
    if (response.IsOk()) {
        // Extract the connection address and port from the response
        std::string::size_type begin =
            std::string(response.GetMessage().String())
                .find_first_of("0123456789");
        if (begin != std::string::npos) {
            uint8 data[6] = {0, 0, 0, 0, 0, 0};
            BString str;
            response.GetMessage().CopyInto(str,
                                           static_cast<int32>(begin),
                                           response.GetMessage().Length());
            std::size_t index = 0;
            for (int i = 0; i < 6; ++i) {
                // Extract the current number
                while (isdigit(str[index])) {
                    data[i] = data[i] * 10 + (str[index] - '0');
                    index++;
                }

                // Skip separator
                index++;
            }

			std::stringstream ss;
			std::string address;

			// Reconstruct connection port and address
			ss	<<	static_cast<uint8>(data[0]) << '.'
				<<	static_cast<uint8>(data[1]) << '.'
				<<	static_cast<uint8>(data[2]) << '.'
				<<	static_cast<uint8>(data[3]) << '.';
			ss >> address;
			int port = data[4] * 256 + data[5];
            
			BNetworkAddress remoteAddr =
                BNetworkAddress(inet_addr(address.c_str()), port);

			if (remoteAddr.InitCheck() != B_OK) {
				return BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);
			}

			delete fDataSocket;

			fDataSocket = new(std::nothrow) BSocket();

			if (fDataSocket == NULL)
				return BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);

			status_t connectError = fDataSocket->Connect(remoteAddr);

            // Connect the data channel to the server
            if (connectError == B_OK) {
                // Translate transfer mode into corresponding FTP parameter
                BString modeStr;
                switch (mode) {
                    case B_FTP_TRANSFER_MODE_BINARY	: modeStr = "I"; break;
                    case B_FTP_TRANSFER_MODE_TEXT	: modeStr = "A"; break;
                    case B_FTP_TRANSFER_MODE_EBCDIC	: modeStr = "E"; break;
                }

                // Set the transfer mode
                response = fFtpConnection.SendCommand("TYPE", modeStr);
            } else {
                // Failed to connect to the server
                response = BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);
            }
        }
    }

    return response;
}



void
BFtpDataChannel::Receive(BDataIO& stream)
{
    char buffer[1024];
    size_t received;

	received = fDataSocket->Read(buffer, sizeof(buffer));
    while (received > 0) {
        stream.Write(buffer, received);
    }
	stream.Flush();

    fDataSocket->Disconnect();
}



void
BFtpDataChannel::Send(std::istream& stream)
{
    // Send data
    char buffer[1024];
    std::size_t count;

    while (true) {
        // read some data from the stream
        stream.read(buffer, sizeof(buffer));

        if (!stream.good() && !stream.eof()) {
            break;
        }

        count = static_cast<std::size_t>(stream.gcount());

        if (count > 0) {
            // we could read more data from the stream: send them
            fDataSocket->Write(buffer, count);
        } else {
            // no more data: exit the loop
            break;
        }
    }

    // Close the data socket
    fDataSocket->Disconnect();
}


