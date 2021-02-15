시간이 촉박하여 현재까지 구현한 함수는 thread_create , thread_exit 함수 입니다.<br>
![image](/uploads/213d983fe1915b1da07f93bf58512890/image.png)
설명을 드리자면 우선 프로세스이긴 하지만 pagedir를 공유하므로, fork 할때 처럼 process를 할당해줍니다.<br>
그 후 쓰레드의 page directory를 현재 프로세스(thread를 불러낸) page directory를 가르키게 합니다.<br>
프로세스의 값들을 복사한 후, user stack을 위해 exec에서 한 것 처럼 stack size를 키우고 직접받아온
arg* 를 user stack에 넣어줍니다.<br> 그리고 스택포인터, esp, eip 들을 함수 실행을 위해 조정해주고, 쓰레드의 state를 RUNNABLE로 만들어 이상이 없다면 return 0을 합니다.<br>
thread_exit 함수는 이전에 짜놓은 exit 함수와 거의 비슷합니다. return value를 따로 다루는것만 다릅니다.<br>