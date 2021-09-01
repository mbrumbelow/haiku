Haiku Modern C++ Guidelines
===========================

This document aims to give guidelines about the use of modern C++ features in
Haiku's system libraries and its applications and add-ons. The target audience
are the contributors to the Haiku project.

This document is not a comprehensive list of all the features in the language,
but rather gives guidance on specific topics. This document will be updated as
insights change or as new (contentious) topics are discussed and decided on.

This document is also not a style format guide, or design guidelines on how to
design and build effective user APIs, though some of the language features will
imply style and API design decisions. For example: the existence of move
semantics implies that it is now possible to prefer scoped objects.

There are four categories/classes of code that this document will be especially
relevant to:

* **Legacy Kits** are kits that were part of the original Be API. These APIs
  have been designed with a subset of C++98 in mind. We speak about a subset,
  because particular features are not used, most notably exceptions. In
  general, any changes to these APIs will continue to be done with that
  style and those conventions.
* **Hybrid Kits** are newer kits that were introduced as part of Haiku. They
  extend the original Be API feature set. Examples are the Locale Kit and the
  Package Kit. The functionality is made availabe for both legacy and
  modern C++ compilers. The current iteration is still using the legacy style
  and conventions, though there are some deviations, like the use of exceptions
  in the Package Kit.
* **Modern Kits** are newer kits that are introduced as a part of Haiku which
  are only available on modern compilers. Currently there are no such Kits in
  the source tree, but it is expected that if WebKit will be made available for
  embedded use in applications, that this will be a Modern Kit.
* Finally, there are **Applications and Add-Ons** that are part of the source
  tree. Almost all target the legacy platform and are built by both the
  legacy and the modern compiler, with the most notable exception of WebPostive
  which is modern C++ only.

This document does not discuss the use of (modern) C++ in the **kernel**,
in **drivers** or as an **Add-On interface** in applications, libraries or
kits.

This document is inspired by
`Google's C++ Style Guide <https://google.github.io/styleguide/cppguide.html>`_
and the
`C++ Core Guidelines <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines>`_.


C++ Version
-----------

The target version for Modern C++ is C++17. This version is supported by the
GCC 8.3 compiler that is bundled with Haiku, but it does need an explicit opt
in by specifying `-std=gnu++17` on the command line when compiling.

Any features from C++20 or later are currently not allowed, unless explicitly
specified in this document.

The target version for our legacy platform is C++98 as supported by GCC 2.95.3.
For public APIs specifically, some language features on the legacy C++98 are
not used, most notably exceptions or the container types from the standard
library in the public interfaces of the kits.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | Subset of C++98 in the public API and in the implementation  |
|              | (with some exceptions as noted in this document)             |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | The public API will primarily be designed with the subset of |
|              | C++98, as a basis, but there may be specific features in     |
|              | C++17 for modern compilers, and some extra C++98 features    |
|              | may be used for consistency                                  |
+--------------+--------------------------------------------------------------+
| Modern Kits  | The public API (and implementation) are implemented in C++17 |
+--------------+--------------------------------------------------------------+
| Applications | For Applications, there is a choice whether they will be     |
|              | developed against the Legacy & Hybrid Kits, or whether they  |
|              | should support the Modern Kits as well. It is not allowed to |
|              | conditionally support Modern Kits, with the exception for    |
|              | applications with legacy Add-Ons that need to be supported.  |
|              | For Add-Ons, the C++ version is determined by the supported  |
|              | platform(s) of the Add-On host.                              |
+--------------+--------------------------------------------------------------+


Copyable and Movable Types
--------------------------

C++11 introduced the ability to move data of one object to another object
without copying. For each of the types in the public API, it should be clear
whether the type supports copying, moving or both. While the standard defines
under which conditions there are `implicit move constructors and assignment
operators <https://en.cppreference.com/w/cpp/language/move_constructor>`, types
must adhere to the `rule of five
<https://cpppatterns.com/patterns/rule-of-five.html>` and make it explicit in
the class definition. This may be done using the ``=default`` or ``=delete``.

The exception for implementing the rule of five, would be for types that
where all data members have public visibility.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | C++98 does not support movable types. There may be scenarios |
|              | in which it makes sense for a type to support move semantics |
|              | for a modern compiler. When this is conditionally enabled    |
|              | for a type, then the implementation must adhere to the rule  |
|              | of five.                                                     |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | When compiled for modern C++, the rule of 5 must be          |
|              | implemented, except for types with only public data members. |
+--------------+--------------------------------------------------------------+
| Modern Kits  | The rule of 5 must be implemented, except for types with     |
|              | only public data members.                                    |
+--------------+--------------------------------------------------------------+
| Applications | Left to the discretion of the author                         |
+--------------+--------------------------------------------------------------+

Agrees with:

* `Google C++: Copyable and Movable <https://google.github.io/styleguide/cppguide.html#Copyable_Movable_Types>`_
* `Guideline C.21 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c21-if-you-define-or-delete-any-copy-move-or-destructor-function-define-or-delete-them-all>`_
* `Guideline C.22 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c22-make-default-operations-consistent>`_

Disagrees (somewhat) with:

* `Guideline C.20 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c20-if-you-can-avoid-defining-default-operations-do>`_


Preference for Scoped Objects
-----------------------------

One of the powerful things of C++ is RAII, where the lifetime of the object
implies that objects are initialized when they come into scope, and that the
resources are freed when they go out of scope. A common problem with unscoped
objects, meaning objects that are explictly allocated and deallocated with
``new/delete`` are memory leaks when a pointer to that object goes out of scope
when the resource has not been explicitly deallocated.

With move semantics, modern C++ has the tools to effectively reduce the use of
non-scope objects in public APIs and their implementations:

* Objects that must be allocated using ``new/delete`` are wrapped in scoped
  objects like ``shared_ptr<T>`` or ``unique_ptr<T>``.
* API functions and methods that create new objects must return scoped
  objects. We can rely on move semantics and compiler optimizations to avoid
  unnecessary allocations.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | When making changes to the existing API, consider whether it |
|              | might be acceptable to prefer return scoped objects even if  |
|              | it involves additional copying of data. In the               |
|              | implementation, use the scoped object guards when possible.  |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | When using modern compilers, in the public API scoped        |
|              | objects must be used where possible. When using the legacy   |
|              | compiler, evaluate whether it makes sense to use scoped      |
|              | objects, even if it involves copying additional data. In the |
|              | implementation, use the scoped object guards when possible.  |
+--------------+--------------------------------------------------------------+
| Modern Kits  | In the public API and the implementation, scoped objects     |
|              | must be used where possible.                                 |
+--------------+--------------------------------------------------------------+
| Applications | Use scoped objects over unscoped objects where you can.      |
+--------------+--------------------------------------------------------------+

Agrees with:

* `Guideline R.5 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-scoped>`_
* `Guideline R.12 <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r12-immediately-give-the-result-of-an-explicit-resource-allocation-to-a-manager-object>`_


NULL and nullptr
----------------

Use ``nullptr`` for assignment and pointer comparison in modern C++.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | Use ``NULL``.                                                |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | Use ``NULL``.                                                |
+--------------+--------------------------------------------------------------+
| Modern Kits  | Use ``nullptr``.                                             |
+--------------+--------------------------------------------------------------+
| Applications | Use ``nullptr``.                                             |
+--------------+--------------------------------------------------------------+

Agrees with:

* `Google C++: nullptr <https://google.github.io/styleguide/cppguide.html#0_and_nullptr/NULL>`_


Usage of Standard Library Containers
------------------------------------

Do not use the containers like ``vector``, ``deque``, or ``set`` in the public
API. Instead use the containers defined by the Haiku API. You are encouraged to
use the standard library containers in the internal workings of the API but do
implement proper exception handling.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | Do not use Standard Libary Containers in the public API.     |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | Do not use Standard Libary Containers in the public API.     |
+--------------+--------------------------------------------------------------+
| Modern Kits  | Do not use Standard Libary Containers in the public API.     |
+--------------+--------------------------------------------------------------+
| Applications | Left to the discretion of the author                         |
+--------------+--------------------------------------------------------------+


Usage of BString in public API
------------------------------

Use ``BString`` in the public API as a return type or as an output parameter
to represent a string, rather than ``std::string`` or ``char`` arrays.
``BString`` is commonly used in the Legacy Kit, and has (slightly) better
UTF-8 support.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | Use ``BString`` in the public API.                           |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | Use ``BString`` in the public API.                           |
+--------------+--------------------------------------------------------------+
| Modern Kits  | Use ``BString`` in the public API.                           |
+--------------+--------------------------------------------------------------+
| Applications | Left to the discretion of the author                         |
+--------------+--------------------------------------------------------------+


Usage of std::string_view in public API
---------------------------------------

Use ``std::string_view`` to pass references/pointers to strings as arguments
or to return (const) references to strings over ``const char*`` or
``const BString&``.

+--------------+--------------------------------------------------------------+
| Legacy Kits  | Use ``const char*`` in the public API.                       |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | Modern C++: use ``std::string_view`` and fall back to        |
|              | `const char*` in legacy C++.                                 |
+--------------+--------------------------------------------------------------+
| Hybrid Kits  | Use ``std::string_view`` exclusively.                        |
+--------------+--------------------------------------------------------------+
| Applications | Left to the discretion of the author                         |
+--------------+--------------------------------------------------------------+


Further topics
--------------

* Error Handling
* Smart Pointers in public API
* constexpr
* noexcept
* Templates and metaprogramming
* std::optional<T>
* std::byte
* std::array
* [[nodiscard]]
