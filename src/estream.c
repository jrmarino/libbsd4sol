/* estream.c - Extended Stream I/O Library
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 g10 Code GmbH
 *
 * This file is part of Libestream.
 *
 * Libestream is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Libestream is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libestream; if not, see <http://www.gnu.org/licenses/>.
 *
 * ALTERNATIVELY, Libestream may be distributed under the terms of the
 * following license, in which case the provisions of this license are
 * required INSTEAD OF the GNU General Public License. If you wish to
 * allow use of your version of this file only under the terms of the
 * GNU General Public License, and not to allow others to use your
 * version of this file under the terms of the following license,
 * indicate your decision by deleting this paragraph and the license
 * below.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <estream.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

/* Generally used types.  */

typedef void *(*func_realloc_t) (void *mem, size_t size);
typedef void (*func_free_t) (void *mem);


/* Buffer management layer.  */

#define BUFFER_BLOCK_SIZE  BUFSIZ
#define BUFFER_UNREAD_SIZE 16


/* Locking.  */
typedef void *estream_mutex_t;

static inline void
dummy_mutex_call_void (estream_mutex_t mutex)
{
  (void)mutex;
}

static inline int
dummy_mutex_call_int (estream_mutex_t mutex)
{
  (void)mutex;
  return 0;
}

#define ESTREAM_MUTEX_INITIALIZER	NULL
#define ESTREAM_MUTEX_LOCK(mutex)	dummy_mutex_call_void ((mutex))
#define ESTREAM_MUTEX_UNLOCK(mutex)	dummy_mutex_call_void ((mutex))
#define ESTREAM_MUTEX_TRYLOCK(mutex)	dummy_mutex_call_int ((mutex))
#define ESTREAM_MUTEX_INITIALIZE(mutex)	dummy_mutex_call_void ((mutex))
#define ESTREAM_SYS_READ  		read
#define ESTREAM_SYS_WRITE 		write
#define ESTREAM_SYS_YIELD() 		do { } while (0)

/* Misc definitions.  */

#ifndef O_BINARY
#define O_BINARY 0
#endif
#define _set_errno(a) do { errno = (a); } while (0)
#define ES_DEFAULT_OPEN_MODE (S_IRUSR | S_IWUSR)

/* A private cookie function to implement an internal IOCTL
   service.  */
typedef int (*cookie_ioctl_function_t) (void *cookie, int cmd,
                                       void *ptr, size_t *len);
/* IOCTL commands for the private cookie function.  */
#define COOKIE_IOCTL_SNATCH_BUFFER 1



/* An internal stream object.  */
struct estream_internal
{
  unsigned char buffer[BUFFER_BLOCK_SIZE];
  unsigned char unread_buffer[BUFFER_UNREAD_SIZE];
  estream_mutex_t lock;		 /* Lock. */
  void *cookie;			 /* Cookie.                */
  void *opaque;			 /* Opaque data.           */
  unsigned int modeflags;	 /* Flags for the backend. */
  char *printable_fname;         /* Malloced filename for es_fname_get.  */
  off_t offset;
  es_cookie_read_function_t func_read;
  es_cookie_write_function_t func_write;
  es_cookie_seek_function_t func_seek;
  cookie_ioctl_function_t func_ioctl;
  es_cookie_close_function_t func_close;
  int strategy;
  int fd;
  struct
  {
    unsigned int err: 1;
    unsigned int eof: 1;
  } indicators;
  unsigned int deallocate_buffer: 1;
  unsigned int is_stdstream:1;   /* This is a standard stream.  */
  unsigned int stdstream_fd:2;   /* 0, 1 or 2 for a standard stream.  */
  unsigned int print_err: 1;     /* Error in print_fun_writer.  */
  unsigned int printable_fname_inuse: 1;  /* es_fname_get has been used.  */
  int print_errno;               /* Errno from print_fun_writer.  */
  size_t print_ntotal;           /* Bytes written from in print_fun_writer. */
  FILE *print_fp;                /* Stdio stream used by print_fun_writer.  */
};


typedef struct estream_internal *estream_internal_t;

#define ESTREAM_LOCK(stream) ESTREAM_MUTEX_LOCK (stream->intern->lock)
#define ESTREAM_UNLOCK(stream) ESTREAM_MUTEX_UNLOCK (stream->intern->lock)
#define ESTREAM_TRYLOCK(stream) ESTREAM_MUTEX_TRYLOCK (stream->intern->lock)

/* Stream list.  */

typedef struct estream_list *estream_list_t;

struct estream_list
{
  estream_t car;
  estream_list_t cdr;
  estream_list_t *prev_cdr;
};

static estream_list_t estream_list;
static estream_mutex_t estream_list_lock;

#define ESTREAM_LIST_LOCK   ESTREAM_MUTEX_LOCK   (estream_list_lock)
#define ESTREAM_LIST_UNLOCK ESTREAM_MUTEX_UNLOCK (estream_list_lock)


/* Macros.  */

/* Evaluate EXPRESSION, setting VARIABLE to the return code, if
   VARIABLE is zero.  */
#define SET_UNLESS_NONZERO(variable, tmp_variable, expression) \
  do                                                           \
    {                                                          \
      tmp_variable = expression;                               \
      if ((! variable) && tmp_variable)                        \
        variable = tmp_variable;                               \
    }                                                          \
  while (0)


/* Malloc wrappers to overcome problems on some older OSes.  */
static void *
mem_alloc (size_t n)
{
  if (!n)
    n++;
  return malloc (n);
}

static void *
mem_realloc (void *p, size_t n)
{
  if (!p)
    return mem_alloc (n);
  return realloc (p, n);
}

static void
mem_free (void *p)
{
  if (p)
    free (p);
}



/*
 * List manipulation.
 */

/* Add STREAM to the list of registered stream objects.  If
   WITH_LOCKED_LIST is true we assumed that the list of streams is
   already locked.  */
static int
es_list_add (estream_t stream, int with_locked_list)
{
  estream_list_t list_obj;
  int ret;

  list_obj = mem_alloc (sizeof (*list_obj));
  if (! list_obj)
    ret = -1;
  else
    {
      if (!with_locked_list)
        ESTREAM_LIST_LOCK;
      list_obj->car = stream;
      list_obj->cdr = estream_list;
      list_obj->prev_cdr = &estream_list;
      if (estream_list)
	estream_list->prev_cdr = &list_obj->cdr;
      estream_list = list_obj;
      if (!with_locked_list)
        ESTREAM_LIST_UNLOCK;
      ret = 0;
    }

  return ret;
}

/* Remove STREAM from the list of registered stream objects.  */
static void
es_list_remove (estream_t stream, int with_locked_list)
{
  estream_list_t list_obj;

  if (!with_locked_list)
    ESTREAM_LIST_LOCK;
  for (list_obj = estream_list; list_obj; list_obj = list_obj->cdr)
    if (list_obj->car == stream)
      {
	*list_obj->prev_cdr = list_obj->cdr;
	if (list_obj->cdr)
	  list_obj->cdr->prev_cdr = list_obj->prev_cdr;
	mem_free (list_obj);
	break;
      }
  if (!with_locked_list)
    ESTREAM_LIST_UNLOCK;
}

/* Type of an stream-iterator-function.  */
typedef int (*estream_iterator_t) (estream_t stream);

/* Iterate over list of registered streams, calling ITERATOR for each
   of them.  */
static int
es_list_iterate (estream_iterator_t iterator)
{
  estream_list_t list_obj;
  int ret = 0;

  ESTREAM_LIST_LOCK;
  for (list_obj = estream_list; list_obj; list_obj = list_obj->cdr)
    ret |= (*iterator) (list_obj->car);
  ESTREAM_LIST_UNLOCK;

  return ret;
}


/*
 * I/O methods.
 */

/* Implementation of Memory I/O.  */

/* Cookie for memory objects.  */
typedef struct estream_cookie_mem
{
  unsigned int modeflags;	/* Open flags.  */
  unsigned char *memory;	/* Allocated data buffer.  */
  size_t memory_size;		/* Allocated size of MEMORY.  */
  size_t memory_limit;          /* Caller supplied maximum allowed
                                   allocation size or 0 for no limit.  */
  size_t offset;		/* Current offset in MEMORY.  */
  size_t data_len;		/* Used length of data in MEMORY.  */
  size_t block_size;		/* Block size.  */
  struct {
    unsigned int grow: 1;	/* MEMORY is allowed to grow.  */
  } flags;
  func_realloc_t func_realloc;
  func_free_t func_free;
} *estream_cookie_mem_t;


/* Create function for memory objects.  DATA is either NULL or a user
   supplied buffer with the initial content of the memory buffer.  If
   DATA is NULL, DATA_N and DATA_LEN need to be 0 as well.  If DATA is
   not NULL, DATA_N gives the allocated size of DATA and DATA_LEN the
   used length in DATA.  */
static int
es_func_mem_create (void *ES__RESTRICT *ES__RESTRICT cookie,
		    unsigned char *ES__RESTRICT data, size_t data_n,
		    size_t data_len,
		    size_t block_size, unsigned int grow,
		    func_realloc_t func_realloc, func_free_t func_free,
		    unsigned int modeflags,
                    size_t memory_limit)
{
  estream_cookie_mem_t mem_cookie;
  int err;

  if (!data && (data_n || data_len))
    {
      _set_errno (EINVAL);
      return -1;
    }

  mem_cookie = mem_alloc (sizeof (*mem_cookie));
  if (!mem_cookie)
    err = -1;
  else
    {
      mem_cookie->modeflags = modeflags;
      mem_cookie->memory = data;
      mem_cookie->memory_size = data_n;
      mem_cookie->memory_limit = memory_limit;
      mem_cookie->offset = 0;
      mem_cookie->data_len = data_len;
      mem_cookie->block_size = block_size;
      mem_cookie->flags.grow = !!grow;
      mem_cookie->func_realloc = func_realloc ? func_realloc : mem_realloc;
      mem_cookie->func_free = func_free ? func_free : mem_free;
      *cookie = mem_cookie;
      err = 0;
    }

  return err;
}

/* Read function for memory objects.  */
static ssize_t
es_func_mem_read (void *cookie, void *buffer, size_t size)
{
  estream_cookie_mem_t mem_cookie = cookie;
  ssize_t ret;

  if (size > mem_cookie->data_len - mem_cookie->offset)
    size = mem_cookie->data_len - mem_cookie->offset;

  if (size)
    {
      memcpy (buffer, mem_cookie->memory + mem_cookie->offset, size);
      mem_cookie->offset += size;
    }

  ret = size;
  return ret;
}


/* Write function for memory objects.  */
static ssize_t
es_func_mem_write (void *cookie, const void *buffer, size_t size)
{
  estream_cookie_mem_t mem_cookie = cookie;
  ssize_t ret;
  size_t nleft;

  if (!size)
    return 0;  /* A flush is a NOP for memory objects.  */

  if (mem_cookie->modeflags & O_APPEND)
    {
      /* Append to data.  */
      mem_cookie->offset = mem_cookie->data_len;
    }

  assert (mem_cookie->memory_size >= mem_cookie->offset);
  nleft = mem_cookie->memory_size - mem_cookie->offset;

  /* If we are not allowed to grow limit the size to the left space.  */
  if (!mem_cookie->flags.grow && size > nleft)
    size = nleft;

  /* Enlarge the memory buffer if needed.  */
  if (size > nleft)
    {
      unsigned char *newbuf;
      size_t newsize;

      if (!mem_cookie->memory_size)
        newsize = size;  /* Not yet allocated.  */
      else
        newsize = mem_cookie->memory_size + (size - nleft);
      if (newsize < mem_cookie->offset)
        {
          _set_errno (EINVAL);
          return -1;
        }

      /* Round up to the next block length.  BLOCK_SIZE should always
         be set; we check anyway.  */
      if (mem_cookie->block_size)
        {
          newsize += mem_cookie->block_size - 1;
          if (newsize < mem_cookie->offset)
            {
              _set_errno (EINVAL);
              return -1;
            }
          newsize /= mem_cookie->block_size;
          newsize *= mem_cookie->block_size;
        }

      /* Check for a total limit.  */
      if (mem_cookie->memory_limit && newsize > mem_cookie->memory_limit)
        {
          _set_errno (ENOSPC);
          return -1;
        }

      newbuf = mem_cookie->func_realloc (mem_cookie->memory, newsize);
      if (!newbuf)
        return -1;

      mem_cookie->memory = newbuf;
      mem_cookie->memory_size = newsize;

      assert (mem_cookie->memory_size >= mem_cookie->offset);
      nleft = mem_cookie->memory_size - mem_cookie->offset;

      assert (size <= nleft);
    }

  memcpy (mem_cookie->memory + mem_cookie->offset, buffer, size);
  if (mem_cookie->offset + size > mem_cookie->data_len)
    mem_cookie->data_len = mem_cookie->offset + size;
  mem_cookie->offset += size;

  ret = size;
  return ret;
}


/* Seek function for memory objects.  */
static int
es_func_mem_seek (void *cookie, off_t *offset, int whence)
{
  estream_cookie_mem_t mem_cookie = cookie;
  off_t pos_new;

  switch (whence)
    {
    case SEEK_SET:
      pos_new = *offset;
      break;

    case SEEK_CUR:
      pos_new = mem_cookie->offset += *offset;
      break;

    case SEEK_END:
      pos_new = mem_cookie->data_len += *offset;
      break;

    default:
      _set_errno (EINVAL);
      return -1;
    }

  if (pos_new > mem_cookie->memory_size)
    {
      size_t newsize;
      void *newbuf;

      if (!mem_cookie->flags.grow)
	{
	  _set_errno (ENOSPC);
	  return -1;
        }

      newsize = pos_new + mem_cookie->block_size - 1;
      if (newsize < pos_new)
        {
          _set_errno (EINVAL);
          return -1;
        }
      newsize /= mem_cookie->block_size;
      newsize *= mem_cookie->block_size;

      if (mem_cookie->memory_limit && newsize > mem_cookie->memory_limit)
        {
          _set_errno (ENOSPC);
          return -1;
        }

      newbuf = mem_cookie->func_realloc (mem_cookie->memory, newsize);
      if (!newbuf)
        return -1;

      mem_cookie->memory = newbuf;
      mem_cookie->memory_size = newsize;
    }

  if (pos_new > mem_cookie->data_len)
    {
      /* Fill spare space with zeroes.  */
      memset (mem_cookie->memory + mem_cookie->data_len,
              0, pos_new - mem_cookie->data_len);
      mem_cookie->data_len = pos_new;
    }

  mem_cookie->offset = pos_new;
  *offset = pos_new;

  return 0;
}

/* Destroy function for memory objects.  */
static int
es_func_mem_destroy (void *cookie)
{
  estream_cookie_mem_t mem_cookie = cookie;

  if (cookie)
    {
      mem_cookie->func_free (mem_cookie->memory);
      mem_free (mem_cookie);
    }
  return 0;
}


static es_cookie_io_functions_t estream_functions_mem =
  {
    es_func_mem_read,
    es_func_mem_write,
    es_func_mem_seek,
    es_func_mem_destroy
  };


static int
es_convert_mode (const char *mode, unsigned int *modeflags)
{
  unsigned int omode, oflags;

  switch (*mode)
    {
    case 'r':
      omode = O_RDONLY;
      oflags = 0;
      break;
    case 'w':
      omode = O_WRONLY;
      oflags = O_TRUNC | O_CREAT;
      break;
    case 'a':
      omode = O_WRONLY;
      oflags = O_APPEND | O_CREAT;
      break;
    default:
      _set_errno (EINVAL);
      return -1;
    }
  for (mode++; *mode; mode++)
    {
      switch (*mode)
        {
        case '+':
          omode = O_RDWR;
          break;
        case 'b':
          oflags |= O_BINARY;
          break;
        case 'x':
          oflags |= O_EXCL;
          break;
        default: /* Ignore unknown flags.  */
          break;
        }
    }

  *modeflags = (omode | oflags);
  return 0;
}

/*
 * Low level stream functionality.
 */

static int
es_fill (estream_t stream)
{
  size_t bytes_read = 0;
  int err;

  if (!stream->intern->func_read)
    {
      _set_errno (EOPNOTSUPP);
      err = -1;
    }
  else
    {
      es_cookie_read_function_t func_read = stream->intern->func_read;
      ssize_t ret;

      ret = (*func_read) (stream->intern->cookie,
			  stream->buffer, stream->buffer_size);
      if (ret == -1)
	{
	  bytes_read = 0;
	  err = -1;
	}
      else
	{
	  bytes_read = ret;
	  err = 0;
	}
    }

  if (err)
    stream->intern->indicators.err = 1;
  else if (!bytes_read)
    stream->intern->indicators.eof = 1;

  stream->intern->offset += stream->data_len;
  stream->data_len = bytes_read;
  stream->data_offset = 0;

  return err;
}

static int
es_flush (estream_t stream)
{
  es_cookie_write_function_t func_write = stream->intern->func_write;
  int err;

  assert (stream->flags.writing);

  if (stream->data_offset)
    {
      size_t bytes_written;
      size_t data_flushed;
      ssize_t ret;

      if (! func_write)
	{
	  err = EOPNOTSUPP;
	  goto out;
	}

      /* Note: to prevent an endless loop caused by user-provided
	 write-functions that pretend to have written more bytes than
	 they were asked to write, we have to check for
	 "(stream->data_offset - data_flushed) > 0" instead of
	 "stream->data_offset - data_flushed".  */

      data_flushed = 0;
      err = 0;

      while ((((ssize_t) (stream->data_offset - data_flushed)) > 0) && (! err))
	{
	  ret = (*func_write) (stream->intern->cookie,
			       stream->buffer + data_flushed,
			       stream->data_offset - data_flushed);
	  if (ret == -1)
	    {
	      bytes_written = 0;
	      err = -1;
	    }
	  else
	    bytes_written = ret;

	  data_flushed += bytes_written;
	  if (err)
	    break;
	}

      stream->data_flushed += data_flushed;
      if (stream->data_offset == data_flushed)
	{
	  stream->intern->offset += stream->data_offset;
	  stream->data_offset = 0;
	  stream->data_flushed = 0;

	  /* Propagate flush event.  */
	  (*func_write) (stream->intern->cookie, NULL, 0);
	}
    }
  else
    err = 0;

 out:

  if (err)
    stream->intern->indicators.err = 1;

  return err;
}

/* Discard buffered data for STREAM.  */
static void
es_empty (estream_t stream)
{
  assert (!stream->flags.writing);
  stream->data_len = 0;
  stream->data_offset = 0;
  stream->unread_data_len = 0;
}

/* Initialize STREAM.  */
static void
es_initialize (estream_t stream,
	       void *cookie, int fd, es_cookie_io_functions_t functions,
               unsigned int modeflags)
{
  stream->intern->cookie = cookie;
  stream->intern->opaque = NULL;
  stream->intern->offset = 0;
  stream->intern->func_read = functions.func_read;
  stream->intern->func_write = functions.func_write;
  stream->intern->func_seek = functions.func_seek;
  stream->intern->func_ioctl = NULL;
  stream->intern->func_close = functions.func_close;
  stream->intern->strategy = _IOFBF;
  stream->intern->fd = fd;
  stream->intern->print_err = 0;
  stream->intern->print_errno = 0;
  stream->intern->print_ntotal = 0;
  stream->intern->print_fp = NULL;
  stream->intern->indicators.err = 0;
  stream->intern->indicators.eof = 0;
  stream->intern->is_stdstream = 0;
  stream->intern->stdstream_fd = 0;
  stream->intern->deallocate_buffer = 0;
  stream->intern->printable_fname = NULL;
  stream->intern->printable_fname_inuse = 0;

  stream->data_len = 0;
  stream->data_offset = 0;
  stream->data_flushed = 0;
  stream->unread_data_len = 0;
  /* Depending on the modeflags we set whether we start in writing or
     reading mode.  This is required in case we are working on a
     stream which is not seeekable (like stdout).  Without this
     pre-initialization we would do a seek at the first write call and
     as this will fail no utput will be delivered. */
  if ((modeflags & O_WRONLY) || (modeflags & O_RDWR) )
    stream->flags.writing = 1;
  else
    stream->flags.writing = 0;
}

/* Deinitialize STREAM.  */
static int
es_deinitialize (estream_t stream)
{
  es_cookie_close_function_t func_close;
  int err, tmp_err;

  if (stream->intern->print_fp)
    {
      int save_errno = errno;
      fclose (stream->intern->print_fp);
      stream->intern->print_fp = NULL;
      _set_errno (save_errno);
    }

  func_close = stream->intern->func_close;

  err = 0;
  if (stream->flags.writing)
    SET_UNLESS_NONZERO (err, tmp_err, es_flush (stream));
  if (func_close)
    SET_UNLESS_NONZERO (err, tmp_err, (*func_close) (stream->intern->cookie));

  mem_free (stream->intern->printable_fname);
  stream->intern->printable_fname = NULL;
  stream->intern->printable_fname_inuse = 0;

  return err;
}

/* Create a new stream object, initialize it.  */
static int
es_create (estream_t *stream, void *cookie, int fd,
	   es_cookie_io_functions_t functions, unsigned int modeflags,
           int with_locked_list)
{
  estream_internal_t stream_internal_new;
  estream_t stream_new;
  int err;

  stream_new = NULL;
  stream_internal_new = NULL;

  stream_new = mem_alloc (sizeof (*stream_new));
  if (! stream_new)
    {
      err = -1;
      goto out;
    }

  stream_internal_new = mem_alloc (sizeof (*stream_internal_new));
  if (! stream_internal_new)
    {
      err = -1;
      goto out;
    }

  stream_new->buffer = stream_internal_new->buffer;
  stream_new->buffer_size = sizeof (stream_internal_new->buffer);
  stream_new->unread_buffer = stream_internal_new->unread_buffer;
  stream_new->unread_buffer_size = sizeof (stream_internal_new->unread_buffer);
  stream_new->intern = stream_internal_new;

  ESTREAM_MUTEX_INITIALIZE (stream_new->intern->lock);
  es_initialize (stream_new, cookie, fd, functions, modeflags);

  err = es_list_add (stream_new, with_locked_list);
  if (err)
    goto out;

  *stream = stream_new;

 out:

  if (err)
    {
      if (stream_new)
	{
	  es_deinitialize (stream_new);
	  mem_free (stream_new);
	}
    }

  return err;
}

/* Deinitialize a stream object and destroy it.  */
static int
es_destroy (estream_t stream, int with_locked_list)
{
  int err = 0;

  if (stream)
    {
      es_list_remove (stream, with_locked_list);
      err = es_deinitialize (stream);
      mem_free (stream->intern);
      mem_free (stream);
    }

  return err;
}

estream_t
es_fopencookie (void *ES__RESTRICT cookie,
		const char *ES__RESTRICT mode,
		es_cookie_io_functions_t functions)
{
  unsigned int modeflags;
  estream_t stream;
  int err;

  stream = NULL;
  modeflags = 0;

  err = es_convert_mode (mode, &modeflags);
  if (err)
    goto out;

  err = es_create (&stream, cookie, -1, functions, modeflags, 0);
  if (err)
    goto out;

 out:

  return stream;
}

int
es_fclose (estream_t stream)
{
  int err;

  err = es_destroy (stream, 0);

  return err;
}

/* Try to read BYTES_TO_READ bytes FROM STREAM into BUFFER in
   unbuffered-mode, storing the amount of bytes read in
   *BYTES_READ.  */
static int
es_read_nbf (estream_t ES__RESTRICT stream,
	     unsigned char *ES__RESTRICT buffer,
	     size_t bytes_to_read, size_t *ES__RESTRICT bytes_read)
{
  es_cookie_read_function_t func_read = stream->intern->func_read;
  size_t data_read;
  ssize_t ret;
  int err;

  data_read = 0;
  err = 0;

  while (bytes_to_read - data_read)
    {
      ret = (*func_read) (stream->intern->cookie,
			  buffer + data_read, bytes_to_read - data_read);
      if (ret == -1)
	{
	  err = -1;
	  break;
	}
      else if (ret)
	data_read += ret;
      else
	break;
    }

  stream->intern->offset += data_read;
  *bytes_read = data_read;

  return err;
}

/* Try to read BYTES_TO_READ bytes FROM STREAM into BUFFER in
   fully-buffered-mode, storing the amount of bytes read in
   *BYTES_READ.  */
static int
es_read_fbf (estream_t ES__RESTRICT stream,
	     unsigned char *ES__RESTRICT buffer,
	     size_t bytes_to_read, size_t *ES__RESTRICT bytes_read)
{
  size_t data_available;
  size_t data_to_read;
  size_t data_read;
  int err;

  data_read = 0;
  err = 0;

  while ((bytes_to_read - data_read) && (! err))
    {
      if (stream->data_offset == stream->data_len)
	{
	  /* Nothing more to read in current container, try to
	     fill container with new data.  */
	  err = es_fill (stream);
	  if (! err)
	    if (! stream->data_len)
	      /* Filling did not result in any data read.  */
	      break;
	}

      if (! err)
	{
	  /* Filling resulted in some new data.  */

	  data_to_read = bytes_to_read - data_read;
	  data_available = stream->data_len - stream->data_offset;
	  if (data_to_read > data_available)
	    data_to_read = data_available;

	  memcpy (buffer + data_read,
		  stream->buffer + stream->data_offset, data_to_read);
	  stream->data_offset += data_to_read;
	  data_read += data_to_read;
	}
    }

  *bytes_read = data_read;

  return err;
}

/* Try to read BYTES_TO_READ bytes FROM STREAM into BUFFER in
   line-buffered-mode, storing the amount of bytes read in
   *BYTES_READ.  */
static int
es_read_lbf (estream_t ES__RESTRICT stream,
	     unsigned char *ES__RESTRICT buffer,
	     size_t bytes_to_read, size_t *ES__RESTRICT bytes_read)
{
  int err;

  err = es_read_fbf (stream, buffer, bytes_to_read, bytes_read);

  return err;
}

/* Try to read BYTES_TO_READ bytes FROM STREAM into BUFFER, storing
   *the amount of bytes read in BYTES_READ.  */
static int
es_readn (estream_t ES__RESTRICT stream,
	  void *ES__RESTRICT buffer_arg,
	  size_t bytes_to_read, size_t *ES__RESTRICT bytes_read)
{
  unsigned char *buffer = (unsigned char *)buffer_arg;
  size_t data_read_unread, data_read;
  int err;

  data_read_unread = 0;
  data_read = 0;
  err = 0;

  if (stream->flags.writing)
    {
      /* Switching to reading mode -> flush output.  */
      err = es_flush (stream);
      if (err)
	goto out;
      stream->flags.writing = 0;
    }

  /* Read unread data first.  */
  while ((bytes_to_read - data_read_unread) && stream->unread_data_len)
    {
      buffer[data_read_unread]
	= stream->unread_buffer[stream->unread_data_len - 1];
      stream->unread_data_len--;
      data_read_unread++;
    }

  switch (stream->intern->strategy)
    {
    case _IONBF:
      err = es_read_nbf (stream,
			 buffer + data_read_unread,
			 bytes_to_read - data_read_unread, &data_read);
      break;
    case _IOLBF:
      err = es_read_lbf (stream,
			 buffer + data_read_unread,
			 bytes_to_read - data_read_unread, &data_read);
      break;
    case _IOFBF:
      err = es_read_fbf (stream,
			 buffer + data_read_unread,
			 bytes_to_read - data_read_unread, &data_read);
      break;
    }

 out:

  if (bytes_read)
    *bytes_read = data_read_unread + data_read;

  return err;
}

int
es_read (estream_t ES__RESTRICT stream,
	 void *ES__RESTRICT buffer, size_t bytes_to_read,
	 size_t *ES__RESTRICT bytes_read)
{
  int err;

  if (bytes_to_read)
    {
      ESTREAM_LOCK (stream);
      err = es_readn (stream, buffer, bytes_to_read, bytes_read);
      ESTREAM_UNLOCK (stream);
    }
  else
    err = 0;

  return err;
}

/* Write BYTES_TO_WRITE bytes from BUFFER into STREAM in
   unbuffered-mode, storing the amount of bytes written in
   *BYTES_WRITTEN.  */
static int
es_write_nbf (estream_t ES__RESTRICT stream,
	      const unsigned char *ES__RESTRICT buffer,
	      size_t bytes_to_write, size_t *ES__RESTRICT bytes_written)
{
  es_cookie_write_function_t func_write = stream->intern->func_write;
  size_t data_written;
  ssize_t ret;
  int err;

  if (bytes_to_write && (! func_write))
    {
      err = EOPNOTSUPP;
      goto out;
    }

  data_written = 0;
  err = 0;

  while (bytes_to_write - data_written)
    {
      ret = (*func_write) (stream->intern->cookie,
			   buffer + data_written,
			   bytes_to_write - data_written);
      if (ret == -1)
	{
	  err = -1;
	  break;
	}
      else
	data_written += ret;
    }

  stream->intern->offset += data_written;
  *bytes_written = data_written;

 out:

  return err;
}

/* Write BYTES_TO_WRITE bytes from BUFFER into STREAM in
   fully-buffered-mode, storing the amount of bytes written in
   *BYTES_WRITTEN.  */
static int
es_write_fbf (estream_t ES__RESTRICT stream,
	      const unsigned char *ES__RESTRICT buffer,
	      size_t bytes_to_write, size_t *ES__RESTRICT bytes_written)
{
  size_t space_available;
  size_t data_to_write;
  size_t data_written;
  int err;

  data_written = 0;
  err = 0;

  while ((bytes_to_write - data_written) && (! err))
    {
      if (stream->data_offset == stream->buffer_size)
	/* Container full, flush buffer.  */
	err = es_flush (stream);

      if (! err)
	{
	  /* Flushing resulted in empty container.  */

	  data_to_write = bytes_to_write - data_written;
	  space_available = stream->buffer_size - stream->data_offset;
	  if (data_to_write > space_available)
	    data_to_write = space_available;

	  memcpy (stream->buffer + stream->data_offset,
		  buffer + data_written, data_to_write);
	  stream->data_offset += data_to_write;
	  data_written += data_to_write;
	}
    }

  *bytes_written = data_written;

  return err;
}


/* Write BYTES_TO_WRITE bytes from BUFFER into STREAM in
   line-buffered-mode, storing the amount of bytes written in
   *BYTES_WRITTEN.  */
static int
es_write_lbf (estream_t ES__RESTRICT stream,
	      const unsigned char *ES__RESTRICT buffer,
	      size_t bytes_to_write, size_t *ES__RESTRICT bytes_written)
{
  size_t data_flushed = 0;
  size_t data_buffered = 0;
  unsigned char *nlp;
  int err = 0;

  nlp = memrchr (buffer, '\n', bytes_to_write);
  if (nlp)
    {
      /* Found a newline, directly write up to (including) this
	 character.  */
      err = es_flush (stream);
      if (!err)
	err = es_write_nbf (stream, buffer, nlp - buffer + 1, &data_flushed);
    }

  if (!err)
    {
      /* Write remaining data fully buffered.  */
      err = es_write_fbf (stream, buffer + data_flushed,
			  bytes_to_write - data_flushed, &data_buffered);
    }

  *bytes_written = data_flushed + data_buffered;
  return err;
}


/* Seek in STREAM.  */
static int
es_seek (estream_t ES__RESTRICT stream, off_t offset, int whence,
	 off_t *ES__RESTRICT offset_new)
{
  es_cookie_seek_function_t func_seek = stream->intern->func_seek;
  int err, ret;
  off_t off;

  if (! func_seek)
    {
      _set_errno (EOPNOTSUPP);
      err = -1;
      goto out;
    }

  if (stream->flags.writing)
    {
      /* Flush data first in order to prevent flushing it to the wrong
	 offset.  */
      err = es_flush (stream);
      if (err)
	goto out;
      stream->flags.writing = 0;
    }

  off = offset;
  if (whence == SEEK_CUR)
    {
      off = off - stream->data_len + stream->data_offset;
      off -= stream->unread_data_len;
    }

  ret = (*func_seek) (stream->intern->cookie, &off, whence);
  if (ret == -1)
    {
      err = -1;
      goto out;
    }

  err = 0;
  es_empty (stream);

  if (offset_new)
    *offset_new = off;

  stream->intern->indicators.eof = 0;
  stream->intern->offset = off;

 out:

  if (err)
    stream->intern->indicators.err = 1;

  return err;
}

/* Write BYTES_TO_WRITE bytes from BUFFER into STREAM in, storing the
   amount of bytes written in BYTES_WRITTEN.  */
static int
es_writen (estream_t ES__RESTRICT stream,
	   const void *ES__RESTRICT buffer,
	   size_t bytes_to_write, size_t *ES__RESTRICT bytes_written)
{
  size_t data_written;
  int err;

  data_written = 0;
  err = 0;

  if (!stream->flags.writing)
    {
      /* Switching to writing mode -> discard input data and seek to
	 position at which reading has stopped.  We can do this only
	 if a seek function has been registered. */
      if (stream->intern->func_seek)
        {
          err = es_seek (stream, 0, SEEK_CUR, NULL);
          if (err)
            {
              if (errno == ESPIPE)
                err = 0;
              else
                goto out;
            }
        }
    }

  switch (stream->intern->strategy)
    {
    case _IONBF:
      err = es_write_nbf (stream, buffer, bytes_to_write, &data_written);
      break;

    case _IOLBF:
      err = es_write_lbf (stream, buffer, bytes_to_write, &data_written);
      break;

    case _IOFBF:
      err = es_write_fbf (stream, buffer, bytes_to_write, &data_written);
      break;
    }

 out:

  if (bytes_written)
    *bytes_written = data_written;
  if (data_written)
    if (!stream->flags.writing)
      stream->flags.writing = 1;

  return err;
}

int
es_write (estream_t ES__RESTRICT stream,
	  const void *ES__RESTRICT buffer, size_t bytes_to_write,
	  size_t *ES__RESTRICT bytes_written)
{
  int err;

  if (bytes_to_write)
    {
      ESTREAM_LOCK (stream);
      err = es_writen (stream, buffer, bytes_to_write, bytes_written);
      ESTREAM_UNLOCK (stream);
    }
  else
    err = 0;

  return err;
}

static int
es_peek (estream_t ES__RESTRICT stream, unsigned char **ES__RESTRICT data,
	 size_t *ES__RESTRICT data_len)
{
  int err;

  if (stream->flags.writing)
    {
      /* Switching to reading mode -> flush output.  */
      err = es_flush (stream);
      if (err)
	goto out;
      stream->flags.writing = 0;
    }

  if (stream->data_offset == stream->data_len)
    {
      /* Refill container.  */
      err = es_fill (stream);
      if (err)
	goto out;
    }

  if (data)
    *data = stream->buffer + stream->data_offset;
  if (data_len)
    *data_len = stream->data_len - stream->data_offset;
  err = 0;

 out:

  return err;
}


/* Skip SIZE bytes of input data contained in buffer.  */
static int
es_skip (estream_t stream, size_t size)
{
  int err;

  if (stream->data_offset + size > stream->data_len)
    {
      _set_errno (EINVAL);
      err = -1;
    }
  else
    {
      stream->data_offset += size;
      err = 0;
    }

  return err;
}

static int
doreadline (estream_t ES__RESTRICT stream, size_t max_length,
            char *ES__RESTRICT *ES__RESTRICT line,
            size_t *ES__RESTRICT line_length)
{
  size_t space_left;
  size_t line_size;
  estream_t line_stream;
  char *line_new;
  void *line_stream_cookie;
  char *newline;
  unsigned char *data;
  size_t data_len;
  int err;

  line_new = NULL;
  line_stream = NULL;
  line_stream_cookie = NULL;

  err = es_func_mem_create (&line_stream_cookie, NULL, 0, 0,
                            BUFFER_BLOCK_SIZE, 1,
                            mem_realloc, mem_free,
                            O_RDWR,
                            0);
  if (err)
    goto out;

  err = es_create (&line_stream, line_stream_cookie, -1,
		   estream_functions_mem, O_RDWR, 0);
  if (err)
    goto out;

  space_left = max_length;
  line_size = 0;
  while (1)
    {
      if (max_length && (space_left == 1))
	break;

      err = es_peek (stream, &data, &data_len);
      if (err || (! data_len))
	break;

      if (data_len > (space_left - 1))
	data_len = space_left - 1;

      newline = memchr (data, '\n', data_len);
      if (newline)
	{
	  data_len = (newline - (char *) data) + 1;
	  err = es_write (line_stream, data, data_len, NULL);
	  if (! err)
	    {
	      space_left -= data_len;
	      line_size += data_len;
	      es_skip (stream, data_len);
	      break;
	    }
	}
      else
	{
	  err = es_write (line_stream, data, data_len, NULL);
	  if (! err)
	    {
	      space_left -= data_len;
	      line_size += data_len;
	      es_skip (stream, data_len);
	    }
	}
      if (err)
	break;
    }
  if (err)
    goto out;

  /* Complete line has been written to line_stream.  */

  if ((max_length > 1) && (! line_size))
    {
      stream->intern->indicators.eof = 1;
      goto out;
    }

  err = es_seek (line_stream, 0, SEEK_SET, NULL);
  if (err)
    goto out;

  if (! *line)
    {
      line_new = mem_alloc (line_size + 1);
      if (! line_new)
	{
	  err = -1;
	  goto out;
	}
    }
  else
    line_new = *line;

  err = es_read (line_stream, line_new, line_size, NULL);
  if (err)
    goto out;

  line_new[line_size] = '\0';

  if (! *line)
    *line = line_new;
  if (line_length)
    *line_length = line_size;

 out:

  if (line_stream)
    es_destroy (line_stream, 0);
  else if (line_stream_cookie)
    es_func_mem_destroy (line_stream_cookie);

  if (err)
    {
      if (! *line)
	mem_free (line_new);
      stream->intern->indicators.err = 1;
    }

  return err;
}

ssize_t
es_getline (char *ES__RESTRICT *ES__RESTRICT lineptr, size_t *ES__RESTRICT n,
	    estream_t ES__RESTRICT stream)
{
  char *line = NULL;
  size_t line_n = 0;
  int err;

  ESTREAM_LOCK (stream);
  err = doreadline (stream, 0, &line, &line_n);
  ESTREAM_UNLOCK (stream);
  if (err)
    goto out;

  if (*n)
    {
      /* Caller wants us to use his buffer.  */

      if (*n < (line_n + 1))
	{
	  /* Provided buffer is too small -> resize.  */

	  void *p;

	  p = mem_realloc (*lineptr, line_n + 1);
	  if (! p)
	    err = -1;
	  else
	    {
	      if (*lineptr != p)
		*lineptr = p;
	    }
	}

      if (! err)
	{
	  memcpy (*lineptr, line, line_n + 1);
	  if (*n != line_n)
	    *n = line_n;
	}
      mem_free (line);
    }
  else
    {
      /* Caller wants new buffers.  */
      *lineptr = line;
      *n = line_n;
    }

 out:

  return err ? err : (ssize_t)line_n;
}
