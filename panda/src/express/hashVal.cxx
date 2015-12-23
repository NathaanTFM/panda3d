// Filename: hashVal.cxx
// Created by:  drose (14Nov00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "hashVal.h"
#include "virtualFileSystem.h"
#include <ctype.h>

#ifdef HAVE_OPENSSL
#include "openSSLWrapper.h"  // must be included before any other openssl.
#include "openssl/md5.h"
#endif  // HAVE_OPENSSL


////////////////////////////////////////////////////////////////////
//     Function: HashVal::output_hex
//       Access: Published
//  Description: Outputs the HashVal as a 32-digit hexadecimal number.
////////////////////////////////////////////////////////////////////
void HashVal::
output_hex(ostream &out) const {
  char buffer[32];
  encode_hex(_hv[0], buffer);
  encode_hex(_hv[1], buffer + 8);
  encode_hex(_hv[2], buffer + 16);
  encode_hex(_hv[3], buffer + 24);
  out.write(buffer, 32);
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::input_hex
//       Access: Published
//  Description: Inputs the HashVal as a 32-digit hexadecimal number.
////////////////////////////////////////////////////////////////////
void HashVal::
input_hex(istream &in) {
  in >> ws;
  char buffer[32];
  size_t i = 0;
  int ch = in.get();

  while (!in.eof() && !in.fail() && isxdigit(ch)) {
    if (i < 32) {
      buffer[i] = (char)ch;
    }
    i++;
    ch = in.get();
  }

  if (i != 32) {
    in.clear(ios::failbit|in.rdstate());
    return;
  }

  if (!in.eof()) {
    in.putback((char)ch);
  } else {
    in.clear();
  }

  decode_hex(buffer, _hv[0]);
  decode_hex(buffer + 8, _hv[1]);
  decode_hex(buffer + 16, _hv[2]);
  decode_hex(buffer + 24, _hv[3]);
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::output_binary
//       Access: Published
//  Description: Outputs the HashVal as a binary stream of bytes in
//               order.  This is not the same order generated by
//               write_stream().
////////////////////////////////////////////////////////////////////
void HashVal::
output_binary(ostream &out) const {
  StreamWriter writer(out);
  writer.add_be_uint32(_hv[0]);
  writer.add_be_uint32(_hv[1]);
  writer.add_be_uint32(_hv[2]);
  writer.add_be_uint32(_hv[3]);
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::input_binary
//       Access: Published
//  Description: Inputs the HashVal as a binary stream of bytes in
//               order.  This is not the same order expected by
//               read_stream().
////////////////////////////////////////////////////////////////////
void HashVal::
input_binary(istream &in) {
  StreamReader reader(in);
  _hv[0] = reader.get_be_uint32();
  _hv[1] = reader.get_be_uint32();
  _hv[2] = reader.get_be_uint32();
  _hv[3] = reader.get_be_uint32();
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::as_dec
//       Access: Published
//  Description: Returns the HashVal as a string with four decimal
//               numbers.
////////////////////////////////////////////////////////////////////
string HashVal::
as_dec() const {
  ostringstream strm;
  output_dec(strm);
  return strm.str();
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::set_from_dec
//       Access: Published
//  Description: Sets the HashVal from a string with four decimal
//               numbers.  Returns true if valid, false otherwise.
////////////////////////////////////////////////////////////////////
bool HashVal::
set_from_dec(const string &text) {
  istringstream strm(text);
  input_dec(strm);
  return !strm.fail();
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::as_hex
//       Access: Published
//  Description: Returns the HashVal as a 32-byte hexadecimal string.
////////////////////////////////////////////////////////////////////
string HashVal::
as_hex() const {
  char buffer[32];
  encode_hex(_hv[0], buffer);
  encode_hex(_hv[1], buffer + 8);
  encode_hex(_hv[2], buffer + 16);
  encode_hex(_hv[3], buffer + 24);
  return string(buffer, 32);
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::set_from_hex
//       Access: Published
//  Description: Sets the HashVal from a 32-byte hexademical string.
//               Returns true if successful, false otherwise.
////////////////////////////////////////////////////////////////////
bool HashVal::
set_from_hex(const string &text) {
  istringstream strm(text);
  input_hex(strm);
  return !strm.fail();
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::as_bin
//       Access: Published
//  Description: Returns the HashVal as a 16-byte binary string.
////////////////////////////////////////////////////////////////////
string HashVal::
as_bin() const {
  Datagram dg;
  write_datagram(dg);
  return dg.get_message();
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::set_from_bin
//       Access: Published
//  Description: Sets the HashVal from a 16-byte binary string.
//               Returns true if successful, false otherwise.
////////////////////////////////////////////////////////////////////
bool HashVal::
set_from_bin(const string &text) {
  nassertr(text.size() == 16, false);
  Datagram dg(text);
  DatagramIterator dgi(dg);
  read_datagram(dgi);
  return true;
}

#ifdef HAVE_OPENSSL
////////////////////////////////////////////////////////////////////
//     Function: HashVal::hash_file
//       Access: Published
//  Description: Generates the hash value from the indicated file.
//               Returns true on success, false if the file cannot be
//               read.  This method is only defined if we have the
//               OpenSSL library (which provides md5 functionality)
//               available.
////////////////////////////////////////////////////////////////////
bool HashVal::
hash_file(const Filename &filename) {
  Filename bin_filename = Filename::binary_filename(filename);
  VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
  istream *istr = vfs->open_read_file(bin_filename, false);
  if (istr == (istream *)NULL) {
    (*this) = HashVal();
    return false;
  }

  bool result = hash_stream(*istr);
  vfs->close_read_file(istr);

  return result;
}
#endif  // HAVE_OPENSSL

#ifdef HAVE_OPENSSL
////////////////////////////////////////////////////////////////////
//     Function: HashVal::hash_stream
//       Access: Published
//  Description: Generates the hash value from the indicated file.
//               Returns true on success, false if the file cannot be
//               read.  This method is only defined if we have the
//               OpenSSL library (which provides md5 functionality)
//               available.
////////////////////////////////////////////////////////////////////
bool HashVal::
hash_stream(istream &stream) {
  unsigned char md[16];

  MD5_CTX ctx;
  MD5_Init(&ctx);

  static const int buffer_size = 1024;
  char buffer[buffer_size];

  // Seek the stream to the beginning in case it wasn't there already.
  stream.seekg(0, ios::beg);

  stream.read(buffer, buffer_size);
  size_t count = stream.gcount();
  while (count != 0) {
    MD5_Update(&ctx, buffer, count);
    stream.read(buffer, buffer_size);
    count = stream.gcount();
  }
  
  // Clear the fail bit so the caller can still read the stream (if it
  // wants to).
  stream.clear();

  MD5_Final(md, &ctx);

  // Store the individual bytes as big-endian ints, from historical
  // convention.
  _hv[0] = (md[0] << 24) | (md[1] << 16) | (md[2] << 8) | (md[3]);
  _hv[1] = (md[4] << 24) | (md[5] << 16) | (md[6] << 8) | (md[7]);
  _hv[2] = (md[8] << 24) | (md[9] << 16) | (md[10] << 8) | (md[11]);
  _hv[3] = (md[12] << 24) | (md[13] << 16) | (md[14] << 8) | (md[15]);

  return true;
}
#endif  // HAVE_OPENSSL


#ifdef HAVE_OPENSSL
////////////////////////////////////////////////////////////////////
//     Function: HashVal::hash_buffer
//       Access: Published
//  Description: Generates the hash value by hashing the indicated
//               data.  This method is only defined if we have the
//               OpenSSL library (which provides md5 functionality)
//               available.
////////////////////////////////////////////////////////////////////
void HashVal::
hash_buffer(const char *buffer, int length) {
  unsigned char md[16];
  MD5((const unsigned char *)buffer, length, md);

  // Store the individual bytes as big-endian ints, from historical
  // convention.
  _hv[0] = (md[0] << 24) | (md[1] << 16) | (md[2] << 8) | (md[3]);
  _hv[1] = (md[4] << 24) | (md[5] << 16) | (md[6] << 8) | (md[7]);
  _hv[2] = (md[8] << 24) | (md[9] << 16) | (md[10] << 8) | (md[11]);
  _hv[3] = (md[12] << 24) | (md[13] << 16) | (md[14] << 8) | (md[15]);
}

#endif  // HAVE_OPENSSL


////////////////////////////////////////////////////////////////////
//     Function: HashVal::encode_hex
//       Access: Private, Static
//  Description: Encodes the indicated unsigned int into an
//               eight-digit hex string, stored at the indicated
//               buffer and the following 8 positions.
////////////////////////////////////////////////////////////////////
void HashVal::
encode_hex(PN_uint32 val, char *buffer) {
  buffer[0] = tohex(val >> 28);
  buffer[1] = tohex(val >> 24);
  buffer[2] = tohex(val >> 20);
  buffer[3] = tohex(val >> 16);
  buffer[4] = tohex(val >> 12);
  buffer[5] = tohex(val >> 8);
  buffer[6] = tohex(val >> 4);
  buffer[7] = tohex(val);
}

////////////////////////////////////////////////////////////////////
//     Function: HashVal::decode_hex
//       Access: Private, Static
//  Description: Decodes the indicated eight-digit hex string into an
//               unsigned integer.
////////////////////////////////////////////////////////////////////
void HashVal::
decode_hex(const char *buffer, PN_uint32 &val) {
  unsigned int bytes[8];
  for (int i = 0; i < 8; i++) {
    bytes[i] = fromhex(buffer[i]);
  }

  val = ((bytes[0] << 28) |
         (bytes[1] << 24) |
         (bytes[2] << 20) |
         (bytes[3] << 16) |
         (bytes[4] << 12) |
         (bytes[5] << 8) |
         (bytes[6] << 4) |
         (bytes[7]));
}

