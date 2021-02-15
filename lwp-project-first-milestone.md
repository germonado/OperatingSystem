a) Process > 실행중인 프로그램
Thread > Process와 비슷하지만 다른 process(thread)와 주소 공간을 공유한다. <br><br>
b)
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
void *(*start_routine)(void *),void *arg);  >  thread 생성

-	thread는 pthread_t 타입 구조체를 가리키는 포인터, 쓰레드와 상호작용하는데 사용됨. 쓰레드 초기화 할 때 ptrhead_create()에 구조체 전달한다.
-	attr은 쓰레드의 속성을 지정하는데 사용, 스택의 크기와 쓰레드의 스케줄링 우선순위 같은 정보를 지정하기 위해서 사용될 수 있음. 개별 속성은 pthread_attr_init() 함수를 호출하여 초기화한다. 보통 NULL 전달
-	(*start_routine)(void *) 이 thread가 실행할 함수를 나타낸다.(함수 포인터) void * 타입의 인자 한 개를 받는다.(start_routine - 함수이름) 그리고 (void *) 값 반환. 만약 void 포인터 타입 대신에 integer를 인자로 사용하는 루틴이라면 void * (*start_routine)(int) 형식임.
-	arg는 실행할 함수에게 전달한 인자를 나타냄,  void 포인터를 start_routine의 함수 인자로 사용하면, 어떤 데이터 타입도 인자로 전달할 수 있고, 반환 값의 타입으로 사용하면 어떤 타입의 결과도 반환할 수 있다.
int pthread_join(pthread_t thread, void **ret_val);  >  다른 thread 완료를 기다리기 위함
-	첫번째 pthread_t 타입 인자는 어떤 thread를 기다리려고 하는지 명시한다. 이 변수는 thread 생성 루틴에 의해 초기화된다(자료구조에 대한 포인터를 pthread_create()의 인자로 전달하여). 이 구조체를 보관해 놓으면, 그 thread가 끝나기를 기다릴 때 사용할 수 있다.
-	ret_val 은 반환값에 대한 포인터, 루틴이 임의의 데이터 타입을 반환할 수 있기 때문에 void에 대한 포인터 타입으로 정의한다. pthread_join() 루틴은 전달된 인자의 값을 변경하기 때문에 값을 전달하는 것이 아니라, 그 값에 대한 포인터를 전달해야 함.

void pthread_exit(void *ret_val);  >  thread 루틴후에 thread 종료 시킴.
<br><br>
c) Pthread와 같은 lwp를 사용하기 위한 아래 함수들을 구현한다.
int thread_create(thread_t * thread, void * (*start_routine)(vo
id *), void *arg);
void thread_exit(void *retval);
int thread_join(thread_t thread, void **retval);
thread는 결국 address space, resource를 공유하는 process라고 볼 수 있기 때문에, fork, exec을 이용해서 과제를 진행하는 것으로 예상한다.
Join은 다른 thread를 기다리니 wait(), exit은 종료하는 exit() 함수를 활용한다.

