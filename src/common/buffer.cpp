/*
 * AIM tools
 * Copyright (C) 2015 lzwdgc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "buffer.h"

#include <stdio.h>

const int build_version =
#include <version.h.in>
;

std::string version()
{
    using namespace std;

    string s;
    s = to_string(0) + "." +
        to_string(1) + "." +
        to_string(0) + "." +
        to_string(build_version);
    return s;
}

std::vector<uint8_t> readFile(const std::string &fn)
{
    FILE *f = fopen(fn.c_str(), "rb");
    if (!f)
    {
        printf("Cannot open file %s\n", fn.c_str());
        throw std::runtime_error("Cannot open file " + fn);
    }
    fseek(f, 0, SEEK_END);
    auto sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(sz);
    fread(buf.data(), 1, sz, f);
    fclose(f);
    return buf;
}

void writeFile(const std::string &fn, const std::vector<uint8_t> &data)
{
    FILE *f = fopen(fn.c_str(), "wb");
    if (!f)
        throw std::runtime_error("Cannot open file " + fn);
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);
}

buffer::buffer()
{
}

buffer::buffer(size_t size)
    : buf_(new std::vector<uint8_t>(size))
{
    size_ = buf_->size();
    skip(0);
}

buffer::buffer(const std::vector<uint8_t> &buf, uint32_t data_offset)
    : buf_(new std::vector<uint8_t>(buf)), data_offset(data_offset)
{
    skip(0);
    size_ = buf_->size();
    end_ = index_ + size_;
}

buffer::buffer(const buffer &rhs, uint32_t size)
    : buf_(rhs.buf_)
{
    index_ = rhs.index_;
    data_offset = rhs.data_offset;
    ptr = rhs.ptr;
    size_ = size;
    end_ = index_ + size_;
    rhs.skip(size);
}

buffer::buffer(const buffer &rhs, uint32_t size, uint32_t offset)
    : buf_(rhs.buf_)
{
    index_ = offset;
    data_offset = offset;
    size_ = size;
    ptr = (uint8_t *)buf_->data() + index_;
    end_ = index_ + size_;
}

std::string buffer::read_string(uint32_t blocksize) const
{
    std::vector<uint8_t> data(blocksize);
    read(data.data(), data.size());
    return (const char *)data.data();
}

std::wstring buffer::read_wstring(uint32_t blocksize) const
{
    std::vector<uint16_t> data(blocksize);
    read(data.data(), data.size());
    return (const wchar_t *)data.data();
}

uint32_t buffer::_read(void *dst, uint32_t size, uint32_t offset) const
{
    if (!buf_)
        throw std::logic_error("buffer: not initialized");
    if (index_ >= end_)
        throw std::logic_error("buffer: out of range");
    if (index_ + offset + size > end_)
        throw std::logic_error("buffer: too much data");
    memcpy(dst, buf_->data() + index_ + offset, size);
    skip(size + offset);
    return size;
}

uint32_t buffer::_write(const void *src, uint32_t size)
{
    if (!buf_)
    {
        buf_ = std::make_shared<std::vector<uint8_t>>(size);
        end_ = size_ = buf_->size();
    }
    if (index_ > end_)
        throw std::logic_error("buffer: out of range");
    if (index_ + size > end_)
    {
        buf_->resize(index_ + size);
        end_ = size_ = buf_->size();
    }
    memcpy((uint8_t *)buf_->data() + index_, src, size);
    skip(size);
    return size;
}

void buffer::skip(int n) const
{
    if (!buf_)
        throw std::logic_error("buffer: not initialized");
    index_ += n;
    data_offset += n;
    ptr = (uint8_t *)buf_->data() + index_;
}

void buffer::reset() const
{
    index_ = 0;
    data_offset = 0;
    ptr = (uint8_t *)buf_->data();
}

void buffer::seek(uint32_t size) const
{
    reset();
    skip(size);
}

bool buffer::check(int index) const
{
    return index_ == index;
}

bool buffer::eof() const
{
    return index_ == end_;
}

uint32_t buffer::index() const
{
    return index_;
}

uint32_t buffer::size() const
{
    return size_;
}

const std::vector<uint8_t> &buffer::buf() const
{
    if (!buf_)
        throw std::logic_error("buffer: not initialized");
    return *buf_;
}
