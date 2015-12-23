# Device interface language

The *device interface language* is a simple set of tags to be added to the middleware class header file. It allows to quickly build a new device from a middleware C++ class.

The idea is to automatically implement the most used read/write patterns (send/receive numbers, strings, arrays, ...).

> Future: It should be quite easy to interface also pure C code with a couple of extra-tags.

## Overview

The language is based on comments added to the header files.

Comments must start with `//>` and they are followed by tags starting with `\`.

The device name is the class name in upper case.

A new operation is added when a tag introduced by a command `//>` is added before a public function of the class. The `\io_type` tag is the only required tag to define a command.

## Example

```cpp
/// @file fridge.hpp

#ifndef __FRIDGE_HPP__
#define __FRIDGE_HPP__

//> \description The kitchen fridge
class Fridge
{
  public:
    Fridge() {}

	//> \description Turn on the fridge
	//> \io_type WRITE
	void turn_on();

	//> \description Turn off the fridge
	//> \io_type WRITE
	void turn_off();

	//> \description Put food into the fridge
	//> \io_type WRITE
	//> \param food The food to be added
	//> \status ERROR_IF_NEG
	//> \on_error Fridge is full !
	int add_food(food_t food);

	//> \description Take food from the fridge
	//> \io_type READ
	//> \param food The food to be retrieved
	//> \status ERROR_IF_NEG
	//> \on_error No such food in the frige. Go shopping !
	int get_food(food_t food);
};

#endif // __FRIDGE_HPP__
```

## Tags

### \io_type  (Required)

Define the type of I/O operation performed.

Options are:
* `WRITE` If the operation is writing/setting data from the client the device
* `READ` If the operation must send data back to the client. The data must be returned by the function.
* `READ_CSTR` If the operation sends en C string (null-terminated `char*`) back to the client.
* `READ_ARRAY src=>len` If the operation must send an array to the client. `src` indicates the origin of the length (`param` or `this`) and `len` is the expression to get the length. For example, if the length is a parameter `size` of the operation then `READ_ARRAY param=>size`, but if the length is obtained by calling the accessor `Size()` of the class then `READ_ARRAY this=>Size()`.
* `WRITE_ARRAY src=>ptr_name src=>len`

> Future: One may extend this to more complex implementations by allowing a sequence of inputs/outputs. This could be describe with a pipe like notation.

> For example a function that take a received buffer, and return an integer:
```cpp
int foo(uint32_t *buffer, size_t len);
```
the code fragment will require to first receive a buffer from the client, then call `foo` with buffer and finally send the result. Which could be described as:
```
//> \io_type (WRITE_ARRAY param=>buffer param=>len) | READ
```

### \description (Optional)

To add a description to the device or a command.

This field is not mandatory but strongly recommended since it can be used for documentation generation.

### \param (Optional)

Parameter description.

Not mandatory but strongly recommended since it can be used for documentation generation.

### \flag (Optional)

Used to indicate some miscellaneous characteristics of an operation.

Defined flags are:
* `AT_INIT` An operation that must be executed when the client initializes the device (typically in the constructor of the client device class)

### \status (Optional)

Indicates the condition for operation execution failing. By default this is set to `NEVER_FAIL`.

It is advisable to set this flag id your function returns an error status and you want KServer to report an execution failure.

Please do **not** use exceptions in your C++ code: KServer won't catch them and won't notice the execution of an operation failed.

Options are:
* `ERROR_IF_NEG` A function returning an `int` fails if the value is negative
* `ERROR_IF_NULL` A function returning a pointer fails if it returns `NULL`
* `NEVER_FAIL` The function never fails (typically returns `void`)

### \on_error (Optional)

Message to log in the server [syslog](syslog.md) in case of execution failure.

### \is_failed (Optional)

Designates a boolean function returning the device status. It must return true when the device is unoperable. A typical example is when memory mapping failed.

The function must not take any argument. This is an optional feature but it is highly recommended to add such a function to your device if it can become inoperable in any way. If this is not defined, KServer will consider that your device never fails.
