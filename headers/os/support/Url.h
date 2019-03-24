/*
 * Copyright 2010-2018 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_H_
#define _B_URL_H_


#include <Archivable.h>
#include <Message.h>
#include <Path.h>
#include <String.h>


class BURL : public BArchivable {
public:
								BURL(const char* url);
								BURL(BMessage* archive);
								BURL(const BURL& other);
								BURL(const BURL& base, const BString& relative);
								BURL(const BPath& path);
								BURL();
	virtual						~BURL();

	// URL fields modifiers
			BURL&				SetUrlString(const BString& url);
			BURL&				SetProtocol(const BString& scheme);
			BURL&				SetUserName(const BString& user);
			BURL&				SetPassword(const BString& password);
			void				SetAuthority(const BString& authority);
			BURL&				SetHost(const BString& host);
			BURL&				SetPort(int port);
			BURL&				SetPath(const BString& path);
			BURL&				SetRequest(const BString& request);
			BURL&				SetFragment(const BString& fragment);

	// URL fields access
			const BString&		UrlString() const;
			const BString&		Protocol() const;
			const BString&		UserName() const;
			const BString&		Password() const;
			const BString&		UserInfo() const;
			const BString&		Host() const;
			int 				Port() const;
			const BString&		Authority() const;
			const BString&		Path() const;
			const BString&		Request() const;
			const BString&		Fragment() const;

	// URL fields tests
			bool 				IsValid() const;
			bool				HasProtocol() const;
			bool				HasUserName() const;
			bool				HasPassword() const;
			bool				HasUserInfo() const;
			bool				HasHost() const;
			bool				HasPort() const;
			bool				HasAuthority() const;
			bool				HasPath() const;
			bool				HasRequest() const;
			bool				HasFragment() const;

	// Url encoding/decoding of needed fields
			void				UrlEncode(bool strict = false);
			void				UrlDecode(bool strict = false);

			status_t			IDNAToAscii();
			status_t			IDNAToUnicode();

	// Url encoding/decoding of strings
	static	BString				UrlEncode(const BString& url,
									bool strict = false,
									bool directory = false);
	static	BString				UrlDecode(const BString& url,
									bool strict = false);

	// utility functionality
			bool				HasPreferredApplication() const;
			BString				PreferredApplication() const;
			status_t			OpenWithPreferredApplication(
									bool onProblemAskUser = true) const;

	// BArchivable members
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	// URL comparison
			bool				operator==(BURL& other) const;
			bool				operator!=(BURL& other) const;

	// URL assignment
			const BURL&			operator=(const BURL& other);
			const BURL&			operator=(const BString& string);
			const BURL&			operator=(const char* string);

	// URL to string conversion
								operator const char*() const;

private:
			void				_ResetFields();
			bool				_ContainsDelimiter(const BString& url);
			status_t			_ExplodeUrlString(const BString& urlString,
									uint32 flags);
			BString				_MergePath(const BString& relative) const;
			void				_SetPathUnsafe(const BString& path);

	static	BString				_DoUrlEncodeChunk(const BString& chunk,
									bool strict, bool directory = false);
	static 	BString				_DoUrlDecodeChunk(const BString& chunk,
									bool strict);

			bool				_IsHostValid() const;
			bool				_IsHostIPV6Valid(size_t offset,
									int32 length) const;
			bool				_IsProtocolValid() const;
	static	bool				_IsUnreserved(char c);
	static	bool				_IsGenDelim(char c);
	static	bool				_IsSubDelim(char c);
	static	bool				_IsIPV6Char(char c);
	static	bool				_IsUsernameChar(char c);
	static	bool				_IsPasswordChar(char c);
	static	bool				_IsHostChar(char c);
	static	bool				_IsPortChar(char c);

	static	void				_RemoveLastPathComponent(BString& path);

			BString				_UrlMimeType() const;

private:
	mutable	BString				fUrlString;
	mutable	BString				fAuthority;
	mutable	BString				fUserInfo;

			BString				fProtocol;
			BString				fUser;
			BString				fPassword;
			BString				fHost;
			int					fPort;
			BString				fPath;
			BString				fRequest;
			BString				fFragment;

	mutable	bool				fUrlStringValid : 1;
	mutable	bool				fAuthorityValid : 1;
	mutable bool				fUserInfoValid : 1;

			bool				fHasProtocol : 1;
			bool				fHasUserName : 1;
			bool				fHasPassword : 1;
			bool				fHasHost : 1;
			bool				fHasPort : 1;
			bool				fHasPath : 1;
			bool				fHasRequest : 1;
			bool				fHasFragment : 1;
};

#endif  // _B_URL_H_
