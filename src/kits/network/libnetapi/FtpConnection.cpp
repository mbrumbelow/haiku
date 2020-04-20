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

#include <Debug.h>
#include <Socket.h>


BFtpResponse::BFtpResponse(int16 code, const std::string& message)
	:
	fStatus(code),
	fMessage(message)
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



const std::string&
BFtpResponse::GetMessage() const
{
    return fMessage;
}



BFtpDirectoryResponse::BFtpDirectoryResponse(const BFtpResponse& response)
	:
	BFtpResponse(response)
{
    if (IsOk()) {
        std::string::size_type begin = GetMessage().find('"', 0);
        std::string::size_type end   = GetMessage().find('"', begin + 1);
        fDirectory = GetMessage().substr(begin + 1, end - begin - 1);
    }
}



const std::string&
BFtpDirectoryResponse::GetDirectory() const
{
    return fDirectory;
}



BFtpListingResponse::BFtpListingResponse(const BFtpResponse& response,
	const std::string& data)
	:
	BFtpResponse(response)
{
    if (IsOk()) {
        std::string::size_type lastPos = 0;
        for (std::string::size_type pos = data.find("\r\n");
			 pos != std::string::npos;
			 pos = data.find("\r\n", lastPos)) {
				fListing.push_back(data.substr(lastPos, pos - lastPos));
				lastPos = pos + 2;
        }
    }
}



const std::vector<std::string>&
BFtpListingResponse::GetListing() const
{
    return fListing;
}



BFtpConnection::BFtpConnection(const BUrl& url, const char* protocolName,
	BUrlProtocolListener* listener, BUrlContext* context)
	:
	BNetworkRequest(url, listener, context, "BUrlProtocol.FTP", protocolName),
	fCommand(kCommandInvalid),
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
		"BUrlProtocol.FTP", "FTTP"),
	fCommand(kCommandInvalid),
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
BFtpConnection::Login(const std::string& name, const std::string& password)
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
BFtpConnection::GetDirectoryListing(const std::string& directory)
{
    fRemotePath = directory;
	fCommand = kCommandGetDirectoryListing;
}



BFtpListingResponse
BFtpConnection::_GetDirectoryListing()
{
    std::ostringstream directoryData;
    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(B_FTP_TRANSFER_MODE_TEXT);
    if (response.IsOk()) {
        response = SendCommand("NLST", fRemotePath);
        if (response.IsOk()) {
            data.Receive(directoryData);
            response = _GetResponse();
        }
    }

    return BFtpListingResponse(response, directoryData.str());
}



void
BFtpConnection::ChangeDirectory(const std::string& directory)
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
BFtpConnection::CreateDirectory(const std::string& name)
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
BFtpConnection::DeleteDirectory(const std::string& name)
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
BFtpConnection::RenameFile(const std::string& file, const std::string& newName)
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
BFtpConnection::DeleteFile(const std::string& name)
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
BFtpConnection::Download(const std::string& remoteFile, const std::string& localPath,
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
            std::string filename = fRemotePath;
            std::string::size_type pos = filename.find_last_of("/\\");
            if (pos != std::string::npos)
                filename = filename.substr(pos + 1);

            std::string path = fLocalPath;
            if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
                path += "/";

            std::ofstream file((path + filename).c_str(), std::ios_base::binary | std::ios_base::trunc);
            if (!file)
                return BFtpResponse(B_FTP_STATUS_INVALID_FILE);

            data.Receive(file);

            file.close();

            response = _GetResponse();

            if (!response.IsOk())
                std::remove((path + filename).c_str());
        }
    }

    return response;
}



void
BFtpConnection::Upload(const std::string& localFile, const std::string& remotePath,
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
    std::ifstream file(fLocalPath.c_str(), std::ios_base::binary);
    if (!file)
        return BFtpResponse(B_FTP_STATUS_INVALID_FILE);

    std::string filename = fLocalPath;
    std::string::size_type pos = filename.find_last_of("/\\");
    if (pos != std::string::npos)
        filename = filename.substr(pos + 1);

    std::string path = fRemotePath;
    if (!path.empty() && (path[path.size() - 1] != '\\') && (path[path.size() - 1] != '/'))
        path += "/";

    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(fMode);
    if (response.IsOk()) {
        response = SendCommand(fAppend ? "APPE" : "STOR", path + filename);
        if (response.IsOk()) {
            data.Send(file);

            response = _GetResponse();
        }
    }

    return response;
}



BFtpResponse
BFtpConnection::SendCommand(const std::string& command, const std::string& parameter)
{
    std::string commandStr;
    if (parameter != "")
        commandStr = command + " " + parameter + "\r\n";
    else
        commandStr = command + "\r\n";

    fSocket->Write(commandStr.c_str(), commandStr.length());

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
        std::size_t length;

        if (fReceiveBuffer.empty()) {
            length = fSocket->Read(buffer, sizeof(buffer));
        } else {
            std::copy(fReceiveBuffer.begin(), fReceiveBuffer.end(), buffer);
            length = fReceiveBuffer.size();
            fReceiveBuffer.clear();
        }

        std::istringstream in(std::string(buffer, length), std::ios_base::binary);
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
                    if ((separator != '-') && ((code == lastCode) || (lastCode == 0))) {
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

                        fReceiveBuffer.assign(buffer + static_cast<std::size_t>(in.tellg()),
							length - static_cast<std::size_t>(in.tellg()));

                        return BFtpResponse(code, message);
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
        std::string::size_type begin = response.GetMessage().find_first_of("0123456789");
        if (begin != std::string::npos) {
            uint8 data[6] = {0, 0, 0, 0, 0, 0};
            std::string str = response.GetMessage().substr(begin);
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
            
			BNetworkAddress remoteAddr = BNetworkAddress(inet_addr(address.c_str()), port);

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
                // Translate the transfer mode to the corresponding FTP parameter
                std::string modeStr;
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
BFtpDataChannel::Receive(std::ostream& stream)
{
    // Receive data
    char buffer[1024];
    std::size_t received;
	received = fDataSocket->Read(buffer, sizeof(buffer));
    while (received > 0) {
        stream.write(buffer, static_cast<std::streamsize>(received));

        if (!stream.good()) {
            break;
        }
    }

    // Close the data socket
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


