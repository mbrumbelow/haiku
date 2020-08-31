/*
 * Copyright 2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_FILE_RESULT_H_
#define _B_FILE_RESULT_H_


#include <FileSession.h>
#include <UrlResult.h>

class BFileResult : public BUrlResult {
public:
	BFileResult(const BUrl& url, BFileSession* session, BFileRequest* request);
	
	virtual	void	Stop();
};

#endif // _B_FILE_RESULT_H_
