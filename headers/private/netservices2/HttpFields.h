/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_FIELDS_H_
#define _B_HTTP_FIELDS_H_

#include <list>
#include <string_view>
#include <variant>
#include <vector>

#include <ErrorsExt.h>
#include <String.h>


namespace BPrivate {

namespace Network {


class BHttpFields {
public:
	// Exceptions
	class InvalidInput : public BError {
	public:
								InvalidInput(const char* origin, BString input);

		virtual	const char*		Message() const noexcept override;
		virtual	BString			DebugMessage() const override;

		BString					input;
	};

	// Wrapper Types
	class FieldName {
	public:
		// Comparison
				bool			operator==(const BString& other) const noexcept;
				bool			operator==(const std::string_view& other) const noexcept;
				bool			operator==(const FieldName& other) const noexcept;

		// Conversion
								operator BString() const;
								operator std::string_view() const;
	private:
		friend	class			BHttpFields;
								FieldName(const std::string_view& name);
								FieldName(BString name);
								FieldName(const FieldName& other);
								FieldName(FieldName&&) noexcept;
		FieldName&				operator=(const FieldName& other);
		FieldName&				operator=(FieldName&&) noexcept;
		FieldName&				operator=(BString name);

		std::variant<std::string_view, BString>	fName;
	};

	class Field {
	public:
		// Constructors
								Field() noexcept;
								Field(const std::string_view& name, const std::string_view& value);
								Field(const Field& other);
								Field(Field&&) noexcept;

		// Assignment
		Field&					operator=(const Field& other);
		Field&					operator=(Field&& other) noexcept;

		// Access Operators
		const FieldName&		Name() const noexcept;
		std::string_view		Value() const noexcept;
		bool					IsEmpty() const noexcept;

	private:
		friend	class			BHttpFields;

								Field(const std::string_view& name, const std::string_view& value, bool borrowed);

		FieldName				fName;
		std::variant<std::string_view, BString>
						fValue;
	};

	// Type Aliases
	using ConstIterator = std::list<Field>::const_iterator;

	// Constructors & Destructor
						BHttpFields();
						BHttpFields(std::initializer_list<Field> fields);
						BHttpFields(const BHttpFields& other);
						BHttpFields(BHttpFields&& other);
						~BHttpFields() noexcept;

	// Assignment operators
	BHttpFields&		operator=(const BHttpFields&);
	BHttpFields&		operator=(BHttpFields&&) noexcept;

	// Access list
	const	Field&		operator[](size_t index) const;

	// Modifiers
	void				AddField(const std::string_view& name, const std::string_view& value);
	void				RemoveField(const std::string_view& name) noexcept; 
	void				RemoveField(ConstIterator it) noexcept;
	void				MakeEmpty() noexcept;

	// Querying
	ConstIterator		FindField(const std::string_view& name) const noexcept;
	size_t				CountFields() const noexcept;

	// Range-based iteration
	ConstIterator		begin() const noexcept;
	ConstIterator		end() const noexcept;

private:
	void				_AddField(Field&& field);

	std::list<Field>	fFields;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_FIELDS_H_

