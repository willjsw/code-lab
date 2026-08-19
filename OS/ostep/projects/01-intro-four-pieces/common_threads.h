#ifndef __common_threads_h__
#define __common_threads_h__

/*
 * common_threads.h — pthread 호출 결과를 assert 로 검증하는 래퍼 매크로.
 *
 * 대문자로 시작하는 Pthread_xxx 는 표준 함수가 아니라 원서 자체 래퍼.
 * pthread 계열 함수는 실패 시 예외가 아니라 0 이 아닌 정수를 반환하므로
 * 반환값을 매번 검사해야 하나, 예제마다 검사 코드를 쓰면 본질이 흐려짐
 * → 매크로로 감싸 반환값 검증을 자동화.
 */

#include <pthread.h>
#include <assert.h>
#include <sched.h>

#ifdef __linux__
#include <semaphore.h>   // macOS 는 이름 없는 POSIX 세마포어 미지원 → Linux 한정
#endif

#define Pthread_create(thread, attr, start_routine, arg) assert(pthread_create(thread, attr, start_routine, arg) == 0);
#define Pthread_join(thread, value_ptr)                  assert(pthread_join(thread, value_ptr) == 0);

#define Pthread_mutex_lock(m)                            assert(pthread_mutex_lock(m) == 0);
#define Pthread_mutex_unlock(m)                          assert(pthread_mutex_unlock(m) == 0);
#define Pthread_cond_signal(cond)                        assert(pthread_cond_signal(cond) == 0);
#define Pthread_cond_wait(cond, mutex)                   assert(pthread_cond_wait(cond, mutex) == 0);

#define Mutex_init(m)                                    assert(pthread_mutex_init(m, NULL) == 0);
#define Mutex_lock(m)                                    assert(pthread_mutex_lock(m) == 0);
#define Mutex_unlock(m)                                  assert(pthread_mutex_unlock(m) == 0);
#define Cond_init(cond)                                  assert(pthread_cond_init(cond, NULL) == 0);
#define Cond_signal(cond)                                assert(pthread_cond_signal(cond) == 0);
#define Cond_wait(cond, mutex)                           assert(pthread_cond_wait(cond, mutex) == 0);

#ifdef __linux__
#define Sem_init(sem, value)                             assert(sem_init(sem, 0, value) == 0);
#define Sem_wait(sem)                                    assert(sem_wait(sem) == 0);
#define Sem_post(sem)                                    assert(sem_post(sem) == 0);
#endif // __linux__

#endif // __common_threads_h__
