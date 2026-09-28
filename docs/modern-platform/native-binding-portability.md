# Binding portability adjustments

SqPlusOverload.h uses SQInteger for release-hook results, matching Squirrel on
LP64 as well as Windows x64. Sqrat DerivedClass qualifies Class<C,A>::ClassWeakref.
Squirrel internals use the existing public sq_type macro rather than exporting
the generic type macro into the standard library. squtils declares allocators
before its templates. These changes retain runtime dispatch and data formats.
