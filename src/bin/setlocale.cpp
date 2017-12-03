/*
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Domonkos Lezsák, lezsakdomi1@gmail.com
 *		Andrew Tockman, andy@keyboardfire.com
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Font.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <MutableLocaleRoster.h>
#include <String.h>

using BPrivate::MutableLocaleRoster;

bool
matches(const char* a, const char* b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a == NULL || b == NULL)
		return false;
	return !strcasecmp(a, b);
}


static const char* kHelpMessage =
	"Set system-wide locale\n"
	"\n"
	"Arguments for long options are mandatory for short options too.\n"
	"Arguments for getting help:\n"
	"  -h, --help    Print usage message, this help text and exit\n"
	"\n"
	"Arguments for listing locales:\n"
	"  -l, --list    Switch to list mode.\n"
	"  -d, --details List in a long, detailed format:\n"
	"                code<TAB>language<TAB>country<TAB>script<TAB>variant<TAB>"
		"name\n"
	"\n"
	"Arguments for setting locale:\n"
	"  -c, --country Specify country for locale\n"
	"  -s, --script  Specify script for locale\n"
	"  -x, --variant Specify language variant\n"
	"\n"
	" Please note, that these can be specified using underscores too.\n"
	" Example:\n"
	"   setlocale en_US\n"
	"   setlocale az_Latn_AZ\n"
	" When multiple locales are specified, the seconds gets set as secondary, "
		"the third as tertiary, etc.\n"
;
static const char* kListModeButTrailingArgsMessage =
	"Error: List mode specified, but it seems like locales too.\n";
static const char* kLocaleNotFound =
	"Warning: Locale %s not found.\n";
static const char* kNoCodeSpecifiedMessage =
	"Error: No language code specified.\n";
static const char* kUnknownArgMessage = "Error: Unknown argument: -%c\n";
static const char* kUsageMessage =
	"Usage: setlocale -l|--list [-d|--details]\n"
	"or     setlocale -h|--help\n"
	"or     setlocale locale_code[ locale_code[ locale_code ...]] "
		"[-c|--country country] [-s|--script script] [-v|--variant variant]\n"
;


void
listLocales(bool detailed = false)
{
	// query for all available languages
	BMessage languages;
	BLocaleRoster::Default()->GetAvailableLanguages(&languages);

	const char* id;
	for (int32 i = 0; languages.FindString("language", i, &id) == B_OK; i++)
	{
		fputs(id, stdout);
		if (detailed) {
			BLanguage* language;
			if (BLocaleRoster::Default()->GetLanguage(id, &language) == B_OK)
			{
				// extract all the relevant information about the language
				const char* code = language->Code();
				const char* country = language->CountryCode();
				const char* script = language->ScriptCode();
				const char* variant = language->Variant();
				BString name; language->GetNativeName(name);

				// print the information
				putchar('\t');
				printf("%s\t", code);
				printf("%s\t", country);
				printf("%s\t", script ? script : "");
				printf("%s\t", variant ? variant : "");
				printf("%s", name.String());

				delete language;
			} else
				fprintf(stderr, "Failed to get BLanguage for %s\n", id);
		}
		putchar('\n');
	}
}


int
main(const int argc, char* const argv[])
{
	bool queryList = false;
	bool queryDetails = false;
	const char* queryCountry = NULL;
	const char* queryScript = NULL;
	const char* queryVariant = NULL;

	// parse command line
	{
		static const struct option longopts[] = {
			{ "help", no_argument, NULL, 'h' },
			{ "list", no_argument, NULL, 'l' },
			{ "details", no_argument, NULL, 'd' },
			{ "country", required_argument, NULL, 'c' },
			{ "script", required_argument, NULL, 's' },
			{ "variant", required_argument, NULL, 'x' },
		};
		const char* optstring = "hldc:s:x:";
		
		for (int c = getopt_long(argc, argv, optstring, longopts, NULL);
			c != -1; c = getopt_long(argc, argv, optstring, longopts, NULL))
		{
			switch (c)
			{
				case 'h':
					fputs(kUsageMessage, stdout);
					fputs(kHelpMessage, stdout);
					return 0;

				case 'l':
					queryList = true;
					break;

				case 'd':
					queryDetails = true;
					break;

				case 'c':
					queryCountry = optarg;
					break;

				case 's':
					queryScript = optarg;
					break;

				case 'x':
					queryVariant = optarg;
					break;

				default:
					// error
					fprintf(stderr, kUnknownArgMessage, c);
					fputc('\n', stderr);
					fputs(kUsageMessage, stderr);
					return 1;
			}
		}

		// Process list mode
		if (queryList) {
			if (optind != argc) {
				fputs(kListModeButTrailingArgsMessage, stderr);
				fputc('\n', stderr);
				fputs(kUsageMessage, stderr);
				return 1;
			}
			listLocales(queryDetails);
			return true;
		}

		// Process language codes
		{
			// Check for number of supplimentary arguments
			if (optind == argc) {
				fputs(kNoCodeSpecifiedMessage, stderr);
				fputc('\n', stderr);
				fputs(kUsageMessage, stderr);
				return 1;
			}

			// And do the job
			{
				BMessage preferred;
				for (int i = optind; i < argc; i++)
				{
					BString targetCode(argv[i]);

					if (queryCountry != NULL)
						targetCode += BString(queryCountry).Prepend('_', 1);

					if (queryVariant != NULL)
						targetCode += BString(queryVariant).Prepend('_', 1);

					if (queryScript != NULL)
						targetCode += BString(queryScript).Prepend('_', 1);

					{
						BLanguage* language;
						if (BLocaleRoster::Default()->GetLanguage(
								targetCode.String(), &language) == B_OK) {
							// found a match! set the new locale
							preferred.AddString("language",
								targetCode.String());
						} else {
							fprintf(stderr, kLocaleNotFound,
								targetCode.String());
						}
					}
				}
				MutableLocaleRoster::Default()->
					SetPreferredLanguages(&preferred);
				return 0;
			}
		}
	}


	// if we've gotten this far, then some error happened
	return 2;
}
