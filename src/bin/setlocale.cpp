/*
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Tockman, andy@keyboardfire.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Font.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <MutableLocaleRoster.h>
#include <String.h>

using BPrivate::MutableLocaleRoster;

bool matches(const char* a, const char* b)
{
	if (a == NULL && b == NULL)
		return true;
	if (a == NULL || b == NULL)
		return false;
	return !strcasecmp(a, b);
}


int main(int argc, char **argv)
{
	const char* queryCode = NULL;
	const char* queryCountry = NULL;
	const char* queryScript = NULL;
	const char* queryVariant = NULL;
	bool list = false, err = false;

	// parse command line
	for (int i = 1; i < argc; ++i) {
		if (*argv[i] == '-') {
			// parse a command line flag
			if (!strcmp(argv[i], "--list"))
				list = true;
			else if (argv[i][1] && !argv[i][2]) {
				// found a single character flag
				if (argv[i][1] == 'l')
					list = true;
				else if (argv[i][1] == 's')
					queryScript = argv[++i];
				else if (argv[i][1] == 'v')
					queryVariant = argv[++i];
				else {
					err = true;
					break;
				}
			} else
				err = false; // unknown flag
		} else if (queryCode == NULL)
			// bare argument; fill up code and then country, erroring on
			// anything more than two arguments
			queryCode = argv[i];
		else if (queryCountry == NULL)
			queryCountry = argv[i];
		else {
			err = true;
			break;
		}
	}

	// check for proper command line, requiring exactly one of list or query
	if (err || ((queryCode == NULL) ^ list)) {
		fprintf(stderr,
			"Usage:\n"
			"  setlocale -l|--list\n"
			"  setlocale language [country] [-s script] [-v variant]\n");
		return 1;
	}

	// query for all available languages
	BMessage languages;
	BLocaleRoster::Default()->GetAvailableLanguages(&languages);

	const char* id;
	for (int32 i = 0; languages.FindString("language", i, &id) == B_OK; ++i) {
		BLanguage* language;
		if (BLocaleRoster::Default()->GetLanguage(id, &language) == B_OK) {
			// extract all the relevant information about the language
			BString name;
			language->GetNativeName(name);
			const char* code = language->Code();
			const char* country = language->CountryCode();
			const char* script = language->ScriptCode();
			const char* variant = language->Variant();

			// if we're listing, print the information
			if (list)
				printf("%s\t%s\t%s\t%s\t%s\n",
					code,
					country ? country : "",
					script ? script : "",
					variant ? variant : "",
					name.String());
			else if (!strcasecmp(code, queryCode)
				&& matches(country, queryCountry)
				&& matches(script, queryScript)
				&& matches(variant, queryVariant)) {
				// found a match! set the new locale
				BMessage preferred;
				preferred.AddString("language", id);
				MutableLocaleRoster::Default()->
					SetPreferredLanguages(&preferred);
				printf("Locale successfully set to %s\n", name.String());
				delete language;
				return 0;
			}

			delete language;
		} else
			fprintf(stderr, "Failed to get BLanguage for %s\n", id);
	}

	// if we've gotten this far and the -l flag wasn't passed, the requested
	// locale wasn't found
	if (!list) {
		fprintf(stderr, "Locale not found\n");
		return 1;
	}

	return 0;
}
