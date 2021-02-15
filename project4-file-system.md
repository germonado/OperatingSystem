Milestone1
=================
*** 참고사항입니다.<br>
제가 예전에 스케쥴링, 스레드 과제를 한 폴더가 xv6-public 폴더인데 거기에 모르고 이번과제를 진행하다가
새로운 xv6에 다시 구현해서 project4로 다시 올렸습니다.<br>
채점하실때 꼭 참고 부탁드립니다.<br>

<br>
마일스톤1은 과제 명세에 나와있는 그림처럼 디자인 하여 구현하였습니다.<br>
1. double indirect 는 128*128<br>
2. triple indirect는 128*128*128의 사이즈를 주었습니다.<br>
<br>
![image](/uploads/d37cf58c344e0ce1ffac91eb3dd075fa/image.png)<br>
![image](/uploads/393a6a95c0448a5c7ca35b0648baa213/image.png)<br>
여기서부터 bmap 함수에 구현한 더블 인다이렉트, 트리플 인다이렉트 코드 부분입니다.<br>
기존에 xv6에서 구현해놓은 indirect를 참고하여 읽어온 block의 entry를 찾고 addr를 다시 읽어오고 할당하는식으로 offset과 entry를 계산하여 할당해주었습니다.<br>
double indirect의 경우 처음 불러온 블락에서 128개의 entry 중 다시 addr블락을 읽어와 할당합니다.<br>
triple indirect는 처음 불러온 블락에서, 다시 한번 block entry를 찾아 불러온 addr에 할당합니다.<br>
어려웠던 부분은 triple을 할당할 때 블락의 블락을 찾아가는 부분에서 entry와 offset이 헷갈려 계산하는데 조금 오래걸렸던 것 같습니다.<br><br>
![image](/uploads/72ad08327b596f104f1f1039ceba1362/image.png)<br>
![image](/uploads/adaac39d64dbb31cd9c43a0cfd74e92b/image.png)<br><br>
여기서부터는 itrunc 함수에 구현한 해제 부분 코드입니다.<br>
새로 포인터를 선언해준 이유는 해제를 할 때 가장 안에있는 block들 부터 해제해주고, 그 다음index로 넘어가서
해제를 해야하기 때문입니다.<br> 그 때 바깥블락을 쥐고있는 포인터를 따로 두고 안을 가리키는 포인터랑 헷갈리지않게 차례대로 해제해줍니다.<br>
double indirect의 경우 위에서 할당해준 순서처럼 들어가 읽어온 후 data 블락부터 순서대로 bfree해줍니다.<br>
그 후 사용한 버퍼 포인터도 해제하고, for문에서 다음 인덱스로 넘어가 하나하나 free해주며 진행합니다.<br>
triple indirect도 같은 방법으로 해제 해줍니다.<br>
아무래도 참고한 xv6 기존 코드가 단순한 버전이라, 읽어오고 해제해주는 부분 순서를 헷갈리거나, 실수로 인자를 잘못넣어줘서 조금 고생했습니다.<br><br>

Milestone2
=================
마일스톤2은 과제 명세에 나와있는 함수들을 고쳐서 구현하였습니다.<br>
1. begin_op()에서는 버퍼가 꽉찬경우 disk로 내리는 것<br>
2. end_op()에서는 버퍼에서 disk로 내리지 못하게 하고, sync()함수에서 disk로 내려줄 것<br><br>
![image](/uploads/5634ea199809e3e24435715498430d80/image.png)

<br>
우선 begin_op() 함수에서 버퍼가 꽉찬경우 sync()를 호출하여 버퍼에있던 data들을 disk로 내려줍니다.<br>
그 후 binit()함수를 불러 버퍼를 초기화 해줍니다.<br>
이 때 lock이 꼬이지 않도록 sync()호출 전에 lock을 해제한 후, 일이 끝난 후에 다시 lock을 잡습니다. <br><br>
![image](/uploads/5aef704194d6ac7cc24e59096fed8878/image.png)<br>
end_op()에서는 commit을 하지 않고, outstanding 값만 조정해줍니다.<br><br>
![image](/uploads/2f3f7b73b69ab82e70db642f3dee8e5c/image.png) <br>
sync()에는 end_op()에서 하던 일을 하도록 구현했습니다.<br>
end_op()에서 하던  commit을 sync()를 호출했을때 실제로 commit을 해줍니다.<br><br>
![image](/uploads/f6d07cef5111f83a42160f8f062e265d/image.png)<br>
그리고 get_log_num() 함수에서는 log.lh.n 값을 리턴해줍니다.<br>
<br>
sync()는 filewrite에서 호출한 경우와 호출하지 않은 경우에 강제종료를 통해서 확인하고, 버퍼가 꽉찬경우는 test_hugefile로 테스트 해보았습니다.<br>

