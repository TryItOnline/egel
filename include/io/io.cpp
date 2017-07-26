#include "../../src/runtime.hpp"

#include "channel.hpp"

#include <stdlib.h>
#include <ostream>
#include <fstream>
#include <memory>

/**
 * Egel's primitive input/output combinators.
 *
 * Unstable and not all combinators fully provide what their 
 * specification promises. Neither are all combinators implemented
 * according to spec.
 *
 * The spec was somewhat shamelessly adapted from ocaml's primitive
 * IO.
 * See https://ocaml.org/.
 **/

// IO.eof
// End of file exception value


// IO.channel
// Values which are input streams
class ChannelValue: public Opaque {
public:
    OPAQUE_PREAMBLE(ChannelValue, "IO", "channel");

    int compare(const VMObjectPtr& o) {
        auto v = (std::static_pointer_cast<ChannelValue>(o))->value();
        if (_value < v) return -1;
        else if (v < _value) return 1;
        else return 0;
    }

    void set_value(ChannelPtr cp) {
        _value = cp;
    }

    ChannelPtr value() {
        return _value;
    }
protected:
    ChannelPtr _value;
};

// IO.cin
// Standard input.
class Stdin: public Medadic {
public:
    MEDADIC_PREAMBLE(Stdin, "IO", "stdin");

    VMObjectPtr apply() const override {
        auto cin = ChannelStreamIn::create();
        auto in  = ChannelValue(machine());
        in.set_value(cin);
        return in.clone();
    }
};

// IO.stdout
// Standard output.
class Stdout: public Medadic {
public:
    MEDADIC_PREAMBLE(Stdout, "IO", "stdout");

    VMObjectPtr apply() const override {
        auto cout = ChannelStreamOut::create();
        auto out  = ChannelValue(machine());
        out.set_value(cout);
        return out.clone();
    }
};

// IO.stderr
// Standard error.
class Stderr: public Medadic {
public:
    MEDADIC_PREAMBLE(Stderr, "IO", "stderr");

    VMObjectPtr apply() const override {
        auto cerr = ChannelStreamOut::create();
        auto err  = ChannelValue(machine());
        err.set_value(cerr);
        return err.clone();
    }
};


// IO.exit n
// Flush all pending writes on stdout and stderr, and terminate the 
// process and return the status code to the operating system.
// (0 to indicate no errors, a small positive integer for failure.)

class Exit: public Monadic {
public:
    MONADIC_PREAMBLE(Exit, "IO", "exit");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        if (arg0->tag() == VM_OBJECT_INTEGER) {
            auto i = VM_OBJECT_INTEGER_VALUE(arg0);
            // XXX: uh.. flushall, or something?
            exit(i);
            // play nice
            return machine()->get_data_string("System", "nop");
        } else {
            return nullptr;
        }
    }
};

// IO.print o
// Print an object on standard output; don't escape characters or 
// strings when they are the argument. May recursively print large
// objects leading to stack explosion.
class Print: public Monadic {
public:
    MONADIC_PREAMBLE(Print, "IO", "print");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {

        static VMObjectPtr nop = nullptr;
        if (nop == nullptr) nop = machine()->get_data_string("System", "nop");

        if (arg0->tag() == VM_OBJECT_INTEGER) {
            auto i = VM_OBJECT_INTEGER_VALUE(arg0);
            std::cout << i;
            return nop;
        } else if (arg0->tag() == VM_OBJECT_FLOAT) {
            auto f = VM_OBJECT_FLOAT_VALUE(arg0);
            std::cout << f;
            return nop;
        } else if (arg0->tag() == VM_OBJECT_CHAR) {
            auto c = VM_OBJECT_CHAR_VALUE(arg0);
            std::cout << c;
            return nop;
        } else if (arg0->tag() == VM_OBJECT_TEXT) {
            auto c = VM_OBJECT_TEXT_VALUE(arg0);
            std::cout << c;
            return nop;
        } else {
            return nullptr;
        }
    }
};

/* Input functions on standard input */

// IO.read_line
// Flush standard output, then read characters from standard input
// until a newline character is encountered. Return the string of all
// characters read, without the newline character at the end.

class Readline: public Medadic {
public:
    MEDADIC_PREAMBLE(Readline, "IO", "readline");

    VMObjectPtr apply() const override {
        std::string line;
        std::getline(std::cin, line);
        UnicodeString str(line.c_str());
        return VMObjectText(str).clone();
    }
};


// IO.readint
// Flush standard output, then read one line from standard input and
// convert it to an integer. Raise Failure "int_of_string" if the line
// read is not a valid representation of an integer.

class Readint: public Medadic {
public:
    MEDADIC_PREAMBLE(Readint, "IO", "readint");

    VMObjectPtr apply() const override {
        vm_int_t n;
        std::cin >> n;
        return VMObjectInteger(n).clone();
    }
};

// IO.readfloat
// Flush standard output, then read one line from standard input and
// convert it to a floating-point number. The result is unspecified if
// the line read is not a valid representation of a floating-point
// number.

class Readfloat: public Medadic {
public:
    MEDADIC_PREAMBLE(Readfloat, "IO", "readfloat");

    VMObjectPtr apply() const override {
        vm_float_t f;
        std::cin >> f;
        return VMObjectFloat(f).clone();
    }
};

/* General output functions */

// IO.open s
// Open the named file for writing, and return a new output channel on
// that file, positionned at the beginning of the file. The file is
// truncated to zero length if it already exists. It is created if it
// does not already exists. Raise sys__Sys_error if the file could not
// be opened.

class Open: public Monadic {
public:
    MONADIC_PREAMBLE(Open, "IO", "open");

    VMObjectPtr apply(const VMObjectPtr& arg0) const override {
        if (arg0->tag() == VM_OBJECT_TEXT) {
            auto fn = VM_OBJECT_TEXT_VALUE(arg0);
            auto stream = ChannelFile::create(fn);
            auto channel  = ChannelValue(machine());
            channel.set_value(stream);
            return channel.clone();
        } else {
            return nullptr;
        }
    }
};

// IO.open_out_bin s
// Same as open_out, but the file is opened in binary mode, so that no
// translation takes place during writes. On operating systems that do
// not distinguish between text mode and binary mode, this function
// behaves like open_out.


// IO.open_out_gen flags i s
// open_out_gen mode rights filename opens the file named filename for
// writing, as above. The extra argument mode specify the opening mode
// (see sys__open). The extra argument rights specifies the file
// permissions, in case the file must be created (see sys__open).
// open_out and open_out_bin are special cases of this function.

// IO.open_descriptor_out fd
// open_descriptor_out fd returns a buffered output channel writing to
// the file descriptor fd. The file descriptor fd must have been
// previously opened for writing, else the behavior is undefined.

// IO.flush channel
// Flush the buffer associated with the given output channel, 
// performing all pending writes on that channel. Interactive programs
// must be careful about flushing std_out and std_err at the right time.

// IO.output_char c
// Write the character on the given output channel.

// IO.output_string s
// Write the string on the given output channel.


// IO.output_byte channel i
// Write one 8-bit integer (as the single character with that code) on
// the given output channel. The given integer is taken modulo 256.

// IO.output_binary_int channel i
// Write one integer in binary format on the given output channel.
// The only reliable way to read it back is through the 
// input_binary_int function. The format is compatible across all
// machines for a given version of Caml Light.

// IO.seek_out channel i
// seek_out chan pos sets the current writing position to pos for
// channel chan. This works only for regular files. On files of other
// kinds (such as terminals, pipes and sockets), the behavior is
// unspecified.

// IO.pos_out channel
// Return the current writing position for the given channel.


// IO.out_channel_length channel
// Return the total length (number of characters) of the given
// channel. This works only for regular files. On files of other
// kinds, the result is meaningless.

// IO.close_out channel
// Close the given channel, flushing all buffered write operations.
// The behavior is unspecified if any of the functions above is called
// on a closed channel.

/* General input functions */

// IO.open_in s
// Open the named file for reading, and return a new input channel on
// that file, positionned at the beginning of the file. Raise
// sys__Sys_error if the file could not be opened.

// IO.open_in_bin s
// Same as open_in, but the file is opened in binary mode, so that no
// translation takes place during reads. On operating systems that do
// not distinguish between text mode and binary mode, this function
// behaves like open_in.

// IO.open_in_gen flags i s
// open_in_gen mode rights filename opens the file named filename for
// reading, as above. The extra arguments mode and rights specify the
// opening mode and file permissions (see sys__open). open_in and
// open_in_bin are special cases of this function.

// IO.open_descriptor_in i
// open_descriptor_in fd returns a buffered input channel reading from
// the file descriptor fd. The file descriptor fd must have been
// previously opened for reading, else the behavior is undefined.

// IO.input_char channel
// Read one character from the given input channel. Raise End_of_file
// if there are no more characters to read.

// IO.input_line channel
// Read characters from the given input channel, until a newline
// character is encountered. Return the string of all characters read,
// without the newline character at the end. Raise End_of_file if the
// end of the file is reached at the beginning of line.

// IO.input_byte channel
// Same as input_char, but return the 8-bit integer representing the
// character. Raise End_of_file if an end of file was reached.

// IO.input_binary_int channel
// Read an integer encoded in binary format from the given input
// channel. See output_binary_int. Raise End_of_file if an end of file
// was reached while reading the integer.

// IO.input_value channel
// Read the representation of a structured value, as produced by
// output_value or output_compact_value, and return the corresponding
// value. This is not type-safe. The type of the returned object is
// not 'a properly speaking: the returned object has one unique type,
// which cannot be determined at compile-time. The programmer should
// explicitly give the expected type of the returned value, using the
// following syntax: (input_value chan : type). The behavior is
// unspecified if the object in the file does not belong to the given
// type.

// IO.seek_in channel n
// seek_in chan pos sets the current reading position to pos for
// channel chan. This works only for regular files. On files of other
// kinds, the behavior is unspecified.

// IO.pos_in channel
// Return the current reading position for the given channel.

// IO.in_channel_length channel
// Return the total length (number of characters) of the given
// channel. This works only for regular files. On files of other
// kinds, the result is meaningless.

// IO.close_in channel
// Close the given channel. Anything can happen if any of the
// functions above is called on a closed channel.

extern "C" std::vector<UnicodeString> egel_imports() {
    return std::vector<UnicodeString>();
}

extern "C" std::vector<VMObjectPtr> egel_exports(VM* vm) {
    std::vector<VMObjectPtr> oo;

    oo.push_back(VMObjectData(vm, "IO", "channel").clone());

    oo.push_back(ChannelValue(vm).clone());
    oo.push_back(Stdin(vm).clone());
    oo.push_back(Stdout(vm).clone());
    oo.push_back(Stderr(vm).clone());
    oo.push_back(Exit(vm).clone());
    oo.push_back(Print(vm).clone());
    oo.push_back(Readline(vm).clone());
    oo.push_back(Readint(vm).clone());
    oo.push_back(Readfloat(vm).clone());
    oo.push_back(Open(vm).clone());

    return oo;

}
