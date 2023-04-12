/*
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLI_DUMP_MEMORY_COMMAND_H
#define CLI_DUMP_MEMORY_COMMAND_H


#include "CliCommand.h"

#include <String.h>


class SourceLanguage;


class CliDumpMemoryCommand : public CliCommand {
public:
								CliDumpMemoryCommand(int32 itemSize,
										const char* itemSizeNoun,
										int32 displayWidth);
	virtual						~CliDumpMemoryCommand();

	virtual	void				Execute(int argc, const char* const* argv,
									CliContext& context);

private:
	SourceLanguage*				fLanguage;
	BString						fSummaryString;
	BString						fUsageString;
	int32						itemSize;
	int32						displayWidth;
};


#endif	// CLI_DUMP_MEMORY_COMMAND_H
