Assignment_Thread
=================

채점하실때 꼭 cpus=1 옵션 주시고 돌려주세요!! 안그러면 프로그램 실행이 되지 않습니다ㅠㅠ(06,22에 추가)

제가 디자인한 Thread는 아래와 같습니다.<br>
![image](/uploads/3f949ac1dcc97f7c019e5fcd7610db08/image.png)
![image](/uploads/098ca0ce27d602fd27d06ddfc709deae/image.png)

![image](/uploads/8f0dbf2fad5487a5bf5eb54a7094e954/image.png)

![image](/uploads/9ebfd48c35e9c53360c45754a489fb09/image.png)

우선 fork, exec 등등 여러 시스템 콜을 참고하여 에러를 잡으면서 스레드를 만들다보니 코드에 주석이 별로 없고 너무 지저분해서 위키를 잘 읽어주시길 바랍니다. 의미 없는 주석이 중복될 수 있는데 exec이나, fork에서 넘어온 것이므로 무시하셔도 됩니다. 직전까지 코드를 짜다보니 주석 제대로 작성하지 못한 점 죄송합니다. 참고하시기 바랍니다.<br>
지난 과제에서 제대로 구현못하고 디자인을 갈아엎어서 달라진 부분이 많습니다. 우선 스레드도 proc으로 다룹니다. 함수는 proc.c에 구현하였습니다. 스레드에 관한 체크표시나 배열이 없다보니 exit, join 할 때 어려움이 생겨 아래에 언급한 것들을 추가하였습니다.<br>
proc.h 에 좀 더 추가한 변수는 tparent, tc, thread[], tret[], tsz[] 등을 추가하였고, 각각이 의미하는 바는 현재 스레드의 부모 (즉 불러낸 main thread)를 가리키는 변수, tc(스레드 체크) 지금 proc이 스레드인가? 맞다면 1로 체크, thread[]배열은 main thread가 나머지 thread를 다룰수 있도록 가리키는 배열(tid를 통해), tret[] tid 별로 각 thread의 리턴 값을 담는 배열, tsz[] 각 스레드의 할당크기 size기억하는 배열(나중에 할당, 해제를 위함) 입니다.<br><br>
제가 말하는 thread parent >> main thread 라고 생각하시면 됩니다.<br>
Thread_create<br>
1. allocproc() 이용하여 프로세스 할당, 그 후 parent와, thread parent , pgdir, tc 변수 각각 설정<br>
2. thread[] 배열에 현재 할당된 스레드 넣고, tid 설정, main thread 스레드 갯수 증가<br>
3. 그후 allocuvm 으로 thread parent stack 사이즈 늘리고 할당해서 스레드 sz로 할당, 버퍼페이지 clear, 파일 복사해줌, trapframe 복사, eip 설정, stack pointer 설정<br>
4. userstack 주소할당, argument넣어줌, copyout으로 userstack 복사, esp 설정, thread 인자에 process(만들어진 스레드) 가리키는 포인터 넣어줌, runnable 세팅!<br><br>

Thread_exit<br>
1. 현재 스레드가 thread parent가 없으면 exit, 아니면 열려있는 파일 닫음<br>
2. 자고 있는 thread parent 깨운다.  이후는 exit 시스템콜과 유사하게 구현<br>
3.  현재 스레드 상태 ZOMBIE로 바꿔주고 thread parent 가 관리하는 tret[] 배열에다가 retval 넣어준다.<br>
4.  thread parent의 thread count값 하나 빼준다.<br><br>

Thread_join<br>
1. join 하려는 스레드가 ZOMBIE가 될 때까지 tparent 재워서 기다렸다가 ZOMBIE 된 후로 kfree로 스택해제,  deallocuvm으로 user,kernel stack 해제합니다<br>
2.  retval 포인터에 tparent의 tret 배열에 들어있는 리턴값 세팅해줍니다.<br>
3.  상태 UNUSED,  tparent의 thread배열에서도 자기가 들어있던 부분 0으로 세팅, kstack, name[0], killed 변수들도 0으로 세팅해줍니다.<br><br>

그리고 나머지 부분은 allocproc, exit, wait 함수부분을 조금씩 수정했습니다. curproc이 thread 인지 main thread인지 구분하여 후자인 경우 돌고있는 thread 수거를 해주는 코드를 넣고, allocproc에서는 새롭게 추가된 thread 배열, 변수들을 세팅해주는 코드를 추가하였습니다.<br><br>

제가 구상한 디자인의 특징과 한계<br><br>
- 우선  제가 기존에 짜놓았던 스케쥴러위에서 돌아가게 구현하려고 하다보니 RR 스케쥴링이 잘 되지 않습니다. 기존의 스케쥴러는 스트라이드가 기본이라 RR 스케쥴링을 하려면 기존의 스케쥴러 함수를 다 뜯어고쳤어야했는데 시간이 촉박하여 그러지 못했습니다.<br> 대신 스레드이면 먼저돌도록 했는데 그 과정에서 처음 만나는 스레드부터 돌게하여 스케쥴링이 조금 이상하게 된 것 같습니다.<br> 스레드가 exit()을 호출하는 것은 괜찮지만, 돌고있는 스레드를 내리는 것, test2에서의 exit, sbrk, stride가 잘 안되고 나머지 시스템콜은 패닉없이 돌아갑니다. <br>
그리고 전체적으로 코딩을 하면서 느낀점은 thread나 scheduler를 구현할때 proc.c 에 한꺼번에  다 같이 구현하지 않았을 걸 하는 아쉬움이 있습니다. 같은곳에서 물리다보니 ptable이나 공유하는 자원이 많아서 아무래도 디버깅을 하는데 난항을 겪었습니다.<br> 또 첫 thread 과제에서 디자인을 제대로 하지 않고 구현하려고 하다보니 잘 안되어 방향이 많이 틀어졌는데, 디자인을 위한 사전 지식, 공부를 꼼꼼히 한후에 코딩을 시작해야겠다는 생각이 많이 들었습니다.