/* Copyright Intel Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#define NUM_THREADS 5

static uv_mutex_t g_mutex;
static uv_thread_t g_thread[NUM_THREADS];

static void locked_pool_thread_cb(void* arg) {
    int r, i = 0;
    do {
      uv_mutex_lock(&g_mutex);
      i++;
      r = uv_run(uv_default_loop(), UV_RUN_NOWAIT);
      uv_mutex_unlock(&g_mutex);
    } while(r > 0);
    ASSERT(r == 0);
    fprintf(stderr, "Thread %p finished %d roundtrips\n", &r, i);
}

static int unlock_calls = 0;
static void locker_cb(uv_loop_t* loop, uv_looplock_mode mode) {
  if(mode == UV_LOOP_LOCK)
    uv_mutex_lock(&g_mutex);
  else if(mode == UV_LOOP_UNLOCK) {
    unlock_calls++;
    uv_mutex_unlock(&g_mutex);
  }
  else ASSERT(0 && "unreachable");

}

static void locked_pool_threads_run(void) {
  int r, i;

  uv_default_loop()->looplock_cb = locker_cb;

  r = uv_mutex_init(&g_mutex);
  ASSERT(r == 0);

  for(i=1; i<NUM_THREADS; i++) {
      r = uv_thread_create(&g_thread[i], locked_pool_thread_cb, NULL);
      ASSERT(r == 0);
  }
  locked_pool_thread_cb(NULL);

  for(i=1; i<NUM_THREADS; i++) {
      r = uv_thread_join(&g_thread[i]);
      ASSERT(r == 0);
  }
  fprintf(stderr, "Unlock calls = %d\n", unlock_calls);
  fflush(stderr);
}
