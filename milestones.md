제가 디자인한 스케줄링은 피아짜 질문과 명세를 참고하여 구상하였습니다.<br>
현재 xv6 scheduler함수는 RR방식으로 timer interrupt를 받아 context-switch가 일어납니다.<br> 하지만 cpu_share(), MLFQ()를 호출하여 스케줄링을 하게 된다면 process 구조체에 MLFQ와 cpu_share를 표시하는 상태변수를 하나 추가하여, xv6 scheduler가 실행될 때 그 값을 확인하여  스케줄링을 할 수 있도록 구현할 생각입니다.<br> 편의상 mlfq(), cpu_share() 호출여부 변수 m_call 이라고 두어 켜진경우 1, m_call을 부르지 않았으면 0.  c_call 변수도 만듭니다. <br> m_num, c_num도 두어, 호출한 process가 몇개인지도 체크합니다.  (stride 계산을 위함)<br> MLFQ는 실행되는 queue마다 시간이 다르기때문에 시간에 tick이 몇번 호출되었는지 체크(변수로 관리)하여 process가 cpu를 포기하도록 trap함수의 yield() 하는 if문을 수정합니다.<br><br>

1.  Stride scheduling  <br>
     - 제가 생각한 stride 스케줄링 구현은 우선 프로세스들이 실행된 후에 pass를 기록하며 1/N 만큼의 cpu를 나눠 가지도록 보장합니다.(proc의 struct에 프로세스 개수들 확인하고 부여, stride 변수도 따로 둔다)<br>
     - 그 후 cpu_share를 call 하는 process에게는 0-20% 사이의 cpu 비율을 보장하도록 합니다.<br>
     - cpu_share를 부른 process는 지정된 cpu 비율을 보장받고, 부르지 않은 process들은 남은 cpu를 나눠서 사용하게 합니다.(stride 재조정)<br><br>

2. MLFQ scheduling <br>
    - MLFQ scheduling을 호출한 경우 3-level로 나누어진 queue에서 각각 돌아가도록 합니다.<br>
    - default tick value는 1tick = 10ms 정도로 되어있는데, 3-level queue에서는 각각 1,2,4 tick 정도의 value로 세팅하려고 합니다.<br>
    - starvation을 막기 위해 10tick 마다 모든 process의 priority를 최상단으로 조정해줍니다.<br>
    - MLFQ는 cpu의 20%를 사용하기 때문에 호출된 경우 MLFQ에 들어있는 process들과 아닌 process들의 stride를 재조정 해줍니다.<br>
    - 만약 MLFQ에 있던 process가 cpu_share를 호출할 경우 MLFQ 스케줄링이 아닌 cpu_share를 통해 확보받은 cpu 비율을 가지게 합니다.(proc 구조체의 상태변수 값 변경) <br>
    - 마찬가지로 cpu_share를 받던 process가 MLFQ방식의 스케줄링 함수를 호출할 경우 자신의 cpu_share를 무시하고 MLFQ 방식으로 스케줄링 하게 합니다. mlfq를 호출한 프로세스의 갯수를 확인하여 stride 값들을 조정해줍니다.<br><br>
