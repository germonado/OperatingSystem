![캡처](/uploads/a81b1e2b5fed00f978ae5c981f6d8a1b/캡처.PNG)
Assignment_OS_Scheduling
=================

제가 디자인한 OS Scheduling의 Stride 스케줄링은 아래와 같습니다.<br>
기본 티켓은 10000개입니다.<br>
1. cpu_share ) 티켓을 기준으로 cpu_share가 요청한 만큼의 stride value(티켓/요청한 cpu 비율)를 주어 스케쥴링을 진행합니다.<br>
2. 기본 stride ) 각 프로세스의 stride value = 티켓/allocproc을 기준으로 세어진 프로세스 갯수<br>
3. MLFQ ) 스케쥴러가 1/5 비율로 MLFQ를 실행해줍니다.(20프로 보장) <br>
4. xv6의 proc.c의 scheduler함수, trap.c 를 수정하고, 나머지는 system call을 구현해서 명세에 있는 함수들로 컨트롤했습니다.<br>
5. proc.c에 boost()라는 함수를 만들어 trap.c 에서 ticks 가 100이 될때마다 boost()를 호출하여 mlfq의 process priority를 0으로 올려주었습니다.<br><br>
제가 구상한 디자인의 특징과 한계<br><br>
> 우선 각 process stride(각 프로세스가 갖는 stride) 는 xv6가 시작될때 할당되는 (shell이 올라오기까지) 의 process도 세어서 나눠줍니다.<br> 이 때문에 기본 stride로 돌아가는 process는 다른 stride 비율을 보장받는 process에 비해 count가 많이 높지 않습니다.(실제로 test_scheduler에서 돌아가는 프로세스의 갯수비율로 보장받지 못합니다.)<br>하지만 cpu_share로 비율을 보장받은 프로세스는 다른 프로세스의 갯수와 상관없이 전체티켓에서 원하는 비율에 맞게 stride 값을 받기때문에 많이 돌게 됩니다.<br> 또한 많이 도는 만큼 5, 15프로가 stride_value값 만큼 정확하게 3배차이가 나지는 않습니다.<br><br>
>  stride_pass 는 돌고있는 프로세스들 중에서 가장 stride_pass가 낮은값으로 설정해주었습니다.<br> 비율이 정확하게 1:3이 아닌이유는 아무래도 우선 기본 process, MLFQ에서 돌아가는 process, cpu_share로 돌아가는 process들의 stride pass 값 등 어느 한쪽의 스케쥴링에서 최솟값을 찾아내 무조건 넣어주는 것은 다른요소가 고려되지 않기 때문에 비율이 제대로 보장되지 않는다고 판단했습니다. <br> 아마 실행시간에 의한 차이, 그리고 실습시간때 yield()를 구현하며 경험했던 I/O 에 의한 delay 차이가 있을 것이라 생각합니다. <br> test_scheduler의 값을 생각하지 않고 디자인한다면 stride_pass 값을 다른 프로세스들의 최소 stride_pass로 보장해주는 게 맞다고 생각합니다.<br><br>
> MLFQ는 process를 만들지 않았기 때문에, MLFQ 스케쥴링을 위해서 ptable.temp를 선언해서 한 프로세스가 돌고 scheduler로 돌아올때마다 5번당 1번꼴로 MLFQ의 process를 실행하도록 하였습니다.<br> 위의 값을 보면 나머지 프로세스들의 count 합과 비교했을때 거의 5번당 1번꼴로 불려지는 것을 볼 수 있습니다.<br>MLFQ에서 레벨별로 정확하게 비율이 보장되지 않는이유는 boost()를 할때 레벨 1, 2에 있는 상태들이 자기에게 할당된 2, 4 tick을 다 못돌고 부스팅을 하게 되면 한번씩 레벨 0, 1만큼 밖에 돌지 않을때가 있어서 정확한 비율이 맞지 않는 것 같습니다.<br><br>
> shell process 만 남은경우(다른 프로그램들이 다 종료된 상태) ptable 에서 사용한 스케쥴링을 위한 변수들을 초기화 해줍니다.<br><br>
> stride_pass 값이 점점 커지다 보면 overflow가 일어나서 stride_pass 값이 맞지 않을 수 있다고 판단하여, boost()함수가 불러질때 각 process의 stride_pass 값을 process 들 중에 가장 작은 stride_pass 값으로 빼주었습니다.<br> 그리고 MLFQ의 경우는 스케쥴링에서 강제적으로 비율을 맞추는 것이라 mlfq 스케쥴링에서 도는 process들은 stride_pass, stride_value 값이 무시됩니다.<br><br>
> 제가 위해서 언급했던 특징과 한계들을 고려하시면서 채점해주시면 정말 감사하겠습니다.
<br><br>

MLFQ 구현)
 제가 과제를 구현하면서 mlfq의 20프로 비율을 맞춰주기 위해 스케쥴러 함수를 위는 일반 stride 아래는 mlfq가 돌아가도록 설계했습니다.<br> 과제를 진행하면서 process 구조체에 여러변수들을 두고 cpu_share인지 일반 stride인지 mlfq스케쥴링에서 돌아가는지 구분을 했습니다. <br> 그러다보니 stride 스케쥴링때는 프로세스 테이블에서 mlfq를 거르고 mlfq 스케쥴링인 경우 mlfq인 프로세스만 식별해서 돌리도록 구현했습니다.(우선순위 0,1,2 순으로 체크해서, queue처럼 돌아가도록)<br> 실제코드에서는 2,1,0 순으로 pointing 하는 프로세스가 바뀌어서 마지막에 0번 priority의 프로세스가 있다면 포인팅 process는 제일 높은 priority의 process로 바뀝니다.<br> 만약 0,1,2 priority 의 프로세스들이 자신이 가진 tick 만큼 돌지못했으면 그 tick만큼 더 돌도록 goto found; 문을 사용해서 낮은 priority여도 자신의 할당 tick만큼 채우도록 해줍니다.
