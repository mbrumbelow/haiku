/*
 * Copyright 2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_FTP_H_
#define _B_URL_PROTOCOL_FTP_H_


#include <iostream>
#include <string>
#include <vector>

#include <DataIO.h>
#include <Path.h>
#include <String.h>
#include <Url.h>
#include <UrlResult.h>
#include <NetworkAddress.h>
#include <NetworkRequest.h>


class BFtpConnection;


class BFtpResponse : public BUrlResult {
	friend	class 				BFtpConnection;

public:
								BFtpResponse(int16 code,
									const BString& message = "");
								BFtpResponse(const BFtpResponse& other);
								BFtpResponse(BMessage*);
								~BFtpResponse();


	virtual	bool 				IsOk() const;
	virtual	int16 				GetStatus() const;
	virtual	const BString&		GetMessage() const;

	BFtpResponse&				operator=(const BFtpResponse& other);

	virtual	status_t			Archive(BMessage*, bool) const;
	static	BArchivable*		Instantiate(BMessage*);

private:
			int16      			fStatus;
			BString 			fMessage;
};


class BFtpDirectoryResponse : public BFtpResponse {
public:
								BFtpDirectoryResponse(const BFtpResponse&
									response);

			const BString&		GetDirectory() const;

private:
			BString				fDirectory;
};


class BFtpListingResponse : public BFtpResponse {
public:
								BFtpListingResponse(const BFtpResponse&
									response,
									const BString& data);

			const std::vector<BString>&
								GetListing() const;

private:
			std::vector<BString>
								fListing;
};


class BFtpDataChannel;


class BFtpConnection : public BNetworkRequest {
	friend	class				BFtpDataChannel;
public:
								BFtpConnection(const BUrl& url,
									const char* protocolName = "FTP",
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
								BFtpConnection(const BFtpConnection& other);
	virtual						~BFtpConnection();

			void				Login();
			void				Login(const BString& name,
									const BString& password);
			void				KeepAlive();
			void				GetWorkingDirectory();
			void				GetDirectoryListing(const BString&
									directory = "");
			void				ChangeDirectory(const BString& directory);
			void				ParentDirectory();
			void				CreateDirectory(const BString& name);
			void				DeleteDirectory(const BString& name);
			void				RenameFile(const BString& file,
									const BString& newName);
			void				DeleteFile(const BString& name);
			void				Download(const BString& remoteFile,
									const BString& localPath,
									int8 mode);
			void				Upload(const BString& localFile,
									const BString& remotePath,
									int8 mode,
									bool append = false);
			BFtpResponse		SendCommand(const BString& command,
									const BString& parameter = "");

			status_t			Connect();
			status_t			Stop();
			const BFtpResponse&	Result() const;

private:
			BFtpResponse		_Login();
			BFtpResponse		_KeepAlive();
			BFtpDirectoryResponse
								_GetWorkingDirectory();
			BFtpListingResponse	_GetDirectoryListing();
			BFtpResponse		_ChangeDirectory();
			BFtpResponse		_ParentDirectory();
			BFtpResponse		_CreateDirectory();
			BFtpResponse		_DeleteDirectory();
			BFtpResponse		_RenameFile();
			BFtpResponse		_DeleteFile();
			BFtpResponse		_Download();
			BFtpResponse		_Upload();


			status_t			_ProtocolLoop();
			BFtpResponse		_GetResponse();

private:
			enum {
				kCommandInvalid,
				kCommandGetWorkingDirectory,
				kCommandGetDirectoryListing,
				kCommandChangeDirectory,
				kCommandParentDirectory,
				kCommandCreateDirectory,
				kCommandDeleteDirectory,
				kCommandRenameFile,
				kCommandDeleteFile,
				kCommandDownload,
				kCommandUpload
			}					fCommand;

			BFtpResponse		fResult;

			BString				fReceiveBuffer;

			int8				fMode;
			BString				fUsername;
			BString				fPassword;
			BPath				fRemotePath;
			BPath				fNewRemotePath;
			BPath				fLocalPath;
			bool				fLogin : 1;
			bool				fKeepAlive : 1;
			bool				fAppend : 1;
};


class BFtpDataChannel {
public:
								BFtpDataChannel(BFtpConnection& owner);

			BFtpResponse		Open(int8 mode);
			void				Send(std::istream& stream);
			void				Receive(BDataIO& stream);

private:
			BFtpConnection&		fFtpConnection;
			BAbstractSocket*	fDataSocket;
};


// Ftp transfer modes
enum ftp_transfer_mode {
	B_FTP_TRANSFER_MODE_BINARY,
	B_FTP_TRANSFER_MODE_TEXT,
	B_FTP_TRANSFER_MODE_EBCDIC
};


// Ftp status classes
enum ftp_status_code_class {
	B_FTP_STATUS_CLASS_INVALID							= 000,
	B_FTP_STATUS_CLASS_POSITIVE_PRELIMINARY 			= 100,
	B_FTP_STATUS_CLASS_POSITIVE_COMPLETION				= 200,
	B_FTP_STATUS_CLASS_POSITIVE_INTERMEDIATE			= 300,
	B_FTP_STATUS_CLASS_TRANSIENT_NEGATIVE_COMPLETION	= 400,
	B_FTP_STATUS_CLASS_PERMANENT_NEGATIVE_COMPLETION	= 500
};


// Ftp status subclasses
enum ftp_status_code_subclass {
	B_FTP_STATUS_SUBCLASS_SYNTAX					= 00,
	B_FTP_STATUS_SUBCLASS_INFORMATION 				= 10,
	B_FTP_STATUS_SUBCLASS_CONNECTIONS				= 20,
	B_FTP_STATUS_SUBCLASS_AUTHENTICATION_ACCOUNTING	= 30,
	B_FTP_STATUS_SUBCLASS_INVALID					= 40,
	B_FTP_STATUS_SUBCLASS_FILE_SYSTEM				= 50
};


// Known FTP status codes
enum ftp_status_code {
	// Request initiating; wait for reply first
	B_FTP_STATUS__POSITIVE_PRELIMINARY_BASE		= 100,
	B_FTP_STATUS_RESTART_MARKER_REPLY			= 110,
	B_FTP_STATUS_SERVICE_READY_SOON				= 120,
	B_FTP_STATUS_DATA_CONNECTION_ALREADY_OPENED	= 125,
	B_FTP_STATUS_OPENING_DATA_CONNECTION		= 150,
	B_FTP_STATUS__POSITIVE_PRELIMINARY_END,

	// Successfuly completed
	B_FTP_STATUS__POSITIVE_COMPLETION_BASE	= 200,
	B_FTP_STATUS_OK	= B_FTP_STATUS__POSITIVE_COMPLETION_BASE,
	B_FTP_STATUS_POINTLESS_COMMAND			= 202,
	B_FTP_STATUS_SYSTEM_STATUS				= 211,
	B_FTP_STATUS_DIRECTORY_STATUS			= 212,
	B_FTP_STATUS_FILE_STATUS				= 213,
	B_FTP_STATUS_HELP_MESSAGE				= 214,
	B_FTP_STATUS_SYSTEM_TYPE				= 215,
	B_FTP_STATUS_SERVICE_READY				= 220,
	B_FTP_STATUS_CLOSING_CONNECTION			= 221,
	B_FTP_STATUS_DATA_CONNECTION_OPENED		= 225,
	B_FTP_STATUS_CLOSING_DATA_CONNECTION	= 226,
	B_FTP_STATUS_ENTERING_PASSIVE_MODE		= 227,
	B_FTP_STATUS_LOGGED_IN					= 230,
	B_FTP_STATUS_FILE_ACTION_OK				= 250,
	B_FTP_STATUS_DIRECTORY_OK				= 257,
	B_FTP_STATUS__POSITIVE_COMPLETION_END,

	// Command accepted; Pending receipt of further information
	B_FTP_STATUS__POSITIVE_INTERMEDIATE_BASE	= 300,
	B_FTP_STATUS_NEED_PASSWORD					= 331,
	B_FTP_STATUS_NEED_ACCOUNT_TO_LOG_IN			= 332,
	B_FTP_STATUS_NEED_INFORMATION				= 350,
	B_FTP_STATUS__POSITIVE_INTERMEDIATE_END,

	// Command not accepted; Can rerequest
	B_FTP_STATUS__TRANSIENT_NEGATIVE_COMPLETION_BASE	= 400,
	B_FTP_STATUS_SERVICE_UNAVAILABLE					= 421,
	B_FTP_STATUS_DATA_CONNECTION_UNAVAILABLE			= 425,
	B_FTP_STATUS_TRANSFER_ABORTED						= 426,
	B_FTP_STATUS_FILE_ACTION_ABORTED					= 450,
	B_FTP_STATUS_LOCAL_ERROR							= 451,
	B_FTP_STATUS_INSUFFICIENT_STORAGE_SPACE				= 452,
	B_FTP_STATUS__TRANSIENT_NEGATIVE_COMPLETION_END,

	// Command not accepted; Cannot rerequest
	B_FTP_STATUS__PERMANENT_NEGATIVE_COMPLETION_BASE	= 500,
	B_FTP_STATUS_COMMAND_UNKONWN =
		B_FTP_STATUS__PERMANENT_NEGATIVE_COMPLETION_BASE,
	B_FTP_STATUS_PARAMETER_UNKNOWN						= 501,
	B_FTP_STATUS_COMMAND_NOT_IMPLEMENTED				= 502,
	B_FTP_STATUS_BAD_COMMAND_SEQUENCE					= 503,
	B_FTP_STATUS_PARAMETER_NOT_IMPLEMENTED				= 504,
	B_FTP_STATUS_NOT_LOGGED_IN							= 530,
	B_FTP_STATUS_NEED_ACCOUNT_TO_STORE					= 532,
	B_FTP_STATUS_FILE_UNAVAILABLE						= 550,
	B_FTP_STATUS_PAGE_TYPE_UNKNOWN						= 551,
	B_FTP_STATUS_NOT_ENOUGH_MEMORY						= 552,
	B_FTP_STATUS_FFILENAME_NOTE_ALLOWED					= 553,
	B_FTP_STATUS__PERMANENT_NEGATIVE_COMPLETION_END,

	// Custom status codes
	B_FTP_STATUS__CUSTOM_BASE		= 1000,
	B_FTP_STATUS_INVALID_RESPONSE = B_FTP_STATUS__CUSTOM_BASE,
	B_FTP_STATUS_CONNECTION_FAILED	= 1001,
	B_FTP_STATUS_CONNECTION_CLOSED	= 1002,
	B_FTP_STATUS_INVALID_FILE		= 1003,
	B_FTP_STATUS__CUSTOM_END
};

#endif // _B_URL_PROTOCOL_FTP_H_
