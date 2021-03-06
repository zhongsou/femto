/*
  (*) 2011-2013 Michael Ferguson <michaelferguson@acm.org>

    * This is a work of the United States Government and is not protected by
      copyright in the United States.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  femto/src/utils_cc/io_pipe.hh
*/
#ifndef _IO_PIPE_HH_
#define _IO_PIPE_HH_

extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include "page_utils.h"
}
#include "pipelining.hh"
#include "file_pipe_context.hh"
#include <memory> // autoptr

// NOTE: these pipes are not thread safe; in other words, only a single thread
// may be using them to read from a file! Multiple threads can easily create
// seperate pipes.

struct buf_info {
  tile t;
  int available;
};

// Manage io buffers...
// This is just shared functionality for io_read_pipe and io_write_pipe.
struct io_manager: private uncopyable {
  pthread_mutex lock; // protects everything here (not used yet).

  // The context! Here we store (more or less) the arguments.
  // We don't copy the context because destructing a file_pipe_context
  // would close the file!
  file_pipe_context ctx;

  struct allocated_pages memory;
  std::vector<buf_info> tiles; // which tiles are available?

  bool closed; // is the pipe closed?

  // Constructors
  // Transfers ownership to here of the file_pipe_context, which should
  // just be arguments..
  io_manager(file_pipe_context ctx, bool write);

  // Allocates tiles once the appropriate member vars have been set.
  void allocate_tiles();
  void flush_and_close();

  void clear_o_direct_if_not_aligned(void* ptr, ssize_t amt, ssize_t offset);

  // Clean up- close the file if we've openned it,
  // wait for all pending AIO to complete
  // move all the requests to 'allocated'
  // Destructor
  ~io_manager();

};

class io_read_pipe: public read_pipe, private uncopyable {
  private:
    io_manager m;
    void make_requests();
  public:
    io_read_pipe(file_pipe_context ctx);

    virtual const tile get_full_tile();
    virtual void put_empty_tile(const tile& t);
    // indicate that no new data will be read -- no new empty tiles added.
    virtual void close_empty();
    // wait for at least one full tile to be placed into the pipe.
    virtual void wait_full();
    // Get the maximum size of each tile
    virtual size_t get_tile_size();
    // Get the number of tiles supported by this pipe.
    virtual size_t get_num_tiles();
    virtual bool is_thread_safe();

    virtual void assert_closed_empty();
};

class io_write_pipe: public write_pipe, private uncopyable {
  private:
    io_manager m;

    void complete_write(struct iocb* cb);

  public:
    io_write_pipe(file_pipe_context ctx);

    virtual tile get_empty_tile();
    virtual void put_full_tile(const tile& t);
    // indicate that no new data will be produced -- no new full tiles.
    virtual void close_full();
    // Return an empty tile if we didn't write anything to it.
    //virtual void return_empty_tile(const tile& t);
    // Get the maximum size of each tile
    virtual size_t get_tile_size();
    // Get the number of tiles supported by this pipe.
    virtual size_t get_num_tiles();
    virtual bool is_thread_safe();

    virtual void assert_closed_full();
};

#endif

