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
#include <fstream>
#include <iterator>
#include <sstream>
#include <cstdio>
#include <sstream>
#include <map>

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



BFtpListingResponse::BFtpListingResponse(const Ftp::Response& response,
	const std::string& data)
	:
	BFtpResponse(response)
{
    if (IsOk()) {
        std::string::size_type lastPos = 0;
        for (std::string::size_type pos = data.find("\r\n");
			pos != std::string::npos;
			pos = data.find("\r\n", lastPos)
			) {
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



FtpConnection::FtpConnection(const BUrl& url, const char* protocolName,
	BUrlProtocolListener* listener, BUrlContext* context)
	:
	BNetworkRequest(url, listener, context, "BUrlProtocol.FTP", protocolName),
	fCommand(""),
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



FtpConnection::FtpConnection(const BFtpConnection& other)
	:
	BNetworkRequest(other.Url(), other.fListener, other.fContext,
		"BUrlProtocol.FTP", "FTTP"),
	fCommand(""),
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



FtpConnection::~FtpConnection()
{
    Stop();

	delete fSocket;
}



void
Ftp::Login()
{
    return Login("anonymous", "haiku-development@freelists.org");
}



void
FtpConnection::Login(const std::string& name, const std::string& password)
{
	fUsername = name;
	fPassword = passwod;
	fLogin = true;
}



BFtpResponse
FtpConnection::_Login()
{
    BFtpResponse response = _SendCommand("USER", fUsername);
    if (response.IsOk())
        response = _SendCommand("PASS", fPassword);

    return response;
}



void
FtpConnection::KeepAlive()
{
    fKeepAlive = true;
}



BFtpResponse
FtpConnection::_KeepAlive()
{
    return _SendCommand("NOOP");
}



void
FtpConnection::GetWorkingDirectory()
{
	fCommand = "GetWorkingDirectory";
}



BFtpDirectoryResponse
FtpConnection::_GetWorkingDirectory()
{
    return DirectoryResponse(_SendCommand("PWD"));
}



void
FtpConnection::GetDirectoryListing(const std::string& directory)
{
    fRemotePath = directory;
	fCommand = "GetDirectoryListing";
}



BFtpListingResponse
FtpConnection::_GetDirectoryListing()
{
    std::ostringstream directoryData;
    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(B_FTP_TRANSFER_MODE_TEXT);
    if (response.IsOk()) {
        response = _SendCommand("NLST", directory);
        if (response.IsOk()) {
            data.Receive(directoryData);
            response = _GetResponse();
        }
    }

    return ListingResponse(response, directoryData.str());
}



void
FtpConnection::ChangeDirectory(const std::string& directory)
{
    fRemotePath = directory;
	fCommand = "ChangeDirectory";
}



BFtpResponse
FtpConnection::_ChangeDirectory()
{
    return _SendCommand("CWD", directory);
}



void
FtpConnection::ParentDirectory()
{
    fCommand = "ParentDirectory";
}



BFtpResponse
FtpConnection::_ParentDirectory()
{
    return _SendCommand("CDUP");
}



void
FtpConnection::CreateDirectory(const std::string& name)
{
    fRemotePath = name;
	fCommand = "CreateDirectory";
}



BFtpResponse
FtpConnection::_CreateDirectory()
{
    return _SendCommand("MKD", fRemotePath);
}



void
FtpConnection::DeleteDirectory(const std::string& name)
{
    fRemotePath = name;
	fCommand = "DeleteDirectory";
}



BFtpResponse
FtpConnection::_DeleteDirectory()
{
    return _SendCommand("RMD", fRemotePath);
}



void
FtpConnection::RenameFile(const std::string& file, const std::string& newName)
{
    fRemotePath = file;
	fNewRemotePath = newName;
	fCommand = "RenameFile";
}



BFtpResponse
FtpConnection::_RenameFile()
{
    BFtpResponse response = _SendCommand("RNFR", fRemotePath);
    if (response.IsOk())
       response = _SendCommand("RNTO", fNewRemotePath);

    return response;
}



void
FtpConnection::DeleteFile(const std::string& name)
{
    fRemotePath = name;
	fCommand = "DeleteFile";
}



BFtpResponse
FtpConnection::_DeleteFile()
{
    return _SendCommand("DELE", fRemotePath);
}


void
FtpConnection::Download(const std::string& remoteFile, const std::string& localPath,
	int8 mode)
{
    fRemotePath = remoteFile;
	fLocalPath = localPath;
	fMode = mode;
	fCommand = "Download";
}



BFtpResponse
FtpConnection::_Download()
{
    BFtpDataChannel data(*this);
    BFtpResponse response = data.Open(fMode);
    if (response.IsOk()) {
        response = _SendCommand("RETR", fRemotePath);
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
FtpConnection::Upload(const std::string& localFile, const std::string& remotePath,
	int8 mode, bool append)
{
	fLocalPath = localFile;
    fRemotePath = remotePath;
	fMode = mode;
	fAppend = append;
	fCommand = "Upload";
}



BFtpResponse
FtpConnection::_Upload()
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
        response = _SendCommand(fAppend ? "APPE" : "STOR", path + filename);
        if (response.IsOk()) {
            data.Send(file);

            response = _GetResponse();
        }
    }

    return response;
}



BFtpResponse
FtpConnection::_SendCommand(const std::string& command, const std::string& parameter)
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
FtpConnection::Connect()
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
FtpConnection::Stop()
{
	BFtpResponse response = _SendCommand("QUIT");
    if (response.IsOk() && fSocket != NULL)
		fSocket->Disconnect();

	return BNetworkRequest::Stop();
}



const BFtpResponse&
FtpConnection::Result() const
{
	return fResult;
}



status_t
BHttpRequest::_ProtocolLoop()
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

    BFtpResponse reponse;

	typedef void (*ftp_method_t)();
    std::map<std::string, ftp_method_t> ftp_method_map;

    ftp_method_map["GetWorkingDirectory"] = &_GetWorkingDirectory();
    ftp_method_map["GetDirectoryListing"] = &_GetDirectoryListing();
    ftp_method_map["ChangeDirectory"    ] = &_ChangeDirectory();
    ftp_method_map["ParentDirectory"    ] = &_ParentDirectory();
    ftp_method_map["CreateDirectory"    ] = &_CreateDirectory();
    ftp_method_map["DeleteDirectory"    ] = &_DeleteDirectory();
    ftp_method_map["RenameFile"         ] = &_RenameFile();
    ftp_method_map["DeleteFile"         ] = &_DeleteFile();
    ftp_method_map["Download"           ] = &_Download();
    ftp_method_map["Upload"             ] = &_Upload();

    std::map<std::string, ftp_method_t>::iterator method = ftp_method_map.find(fCommand);
    if (method != ftp_method_map.end())
        reponse = (*method->second)();
    else
        return B_RESOURCE_NOT_FOUND;

    if (!reponse.IsOk())
        return B_RESOURCE_UNAVAILABLE;

	return B_OK;
}



BFtpResponse
FtpConnection::_GetResponse()
{
    unsigned int lastCode  = 0;
    bool isInsideMultiline = false;
    std::string message;

    for (;;) {
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
            int16 code;
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



BFtpDataChannel::BFtpDataChannel(FtpConnection& owner)
	:
	fFtp(owner)
{
}



BFtpResponse
BFtpDataChannel::Open(int8 mode)
{
    // Open a data connection in active mode (we connect to the server)
    BFtpResponse response = fFtp._SendCommand("PASV");
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
			std::string host;

			// Reconstruct connection port and address
			ss	<<	static_cast<uint8>(data[0]) << '.',
				<<	static_cast<uint8>(data[1]) << '.',
				<<	static_cast<uint8>(data[2]) << '.',
				<<	static_cast<uint8>(data[3]) << '.';
			ss >> host;
			int port = data[4] * 256 + data[5];
            
			BNetworkAddress remoteAddr = BNetworkAddress(host, port);

			if (fRemoteAddr.InitCheck() != B_OK) {
				_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR,
					"Unable to resolve hostname (%s), aborting.",
						host);
				return BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);
			}

			delete fDataSocket;

			fDataSocket = new(std::nothrow) BSocket();

			if (fDataSocket == NULL)
				return BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);

			_EmitDebug(B_URL_PROTOCOL_DEBUG_TEXT, "Connection to %s on port %d.",
				fUrl.Authority().String(), fRemoteAddr.Port());
			status_t connectError = fSocket->Connect(fRemoteAddr);

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
                response = fFtp._SendCommand("TYPE", modeStr);
            } else {
                // Failed to connect to the server
				_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "Socket connection error %s",
					strerror(connectError));

                response = FtpConnection::BFtpResponse(B_FTP_STATUS_CONNECTION_FAILED);
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
			_EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "FTP Error: Writing to the file has failed");
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

    for (;;) {
        // read some data from the stream
        stream.read(buffer, sizeof(buffer));

        if (!stream.good() && !stream.eof()) {
            _EmitDebug(B_URL_PROTOCOL_DEBUG_ERROR, "FTP Error: Reading to the file has failed");
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


