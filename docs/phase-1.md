> **페이즈 1(2026-09-08) 시점의 기록이다.** 그 뒤의 변경은 반영하지 않았다 —
> 폴더 구조, 배치 수식의 위치(C++로 이관), 라운드 종료 화면, TODO 항목 몇 개가 달라졌다.
> 현재 상태는 저장소 루트의 [README](../README.md)를 볼 것.

# Little Cooks

**2~4인 코믹 물리 협동 게임** — 거대 주방, 젓가락 두 짝. 그리고 밥알 500개.

제한 시간 안에, 완성도가 높은 뭉치를 최대한 많이 납품하는 게임이다.
요정 크기 캐릭터가 젓가락으로 밥알을 하나씩 모아 뭉쳐서 초밥을 만든다.

`Unreal Engine 5.6` · 블루프린트 · 리슨 서버 · 개인 프로젝트

---

## 이 프로젝트가 답하는 질문

밥알 500개가 각각 물리 액터다. **최대 4인 멀티플레이에서 전부 복제하면 대역폭이 버티지 못하고, 화면마다 모양이 달라진다.**

대역폭만의 문제가 아니다. 협동 게임에서 "내가 뭉친 밥이 상대 화면에서 다르게 생겼다"는 게임의 전제 자체를 무너뜨린다.

## 선택 — 복제하는 대신, 같은 답을 각자 계산한다

서버가 시드 하나를 복제하고, 각 머신이 **같은 난수 스트림 + 라인 트레이스**로 조리대 위에 밥알을 재현한다.

```
서버                          클라이언트
SpawnSeed  ──── 복제 ────▶   SpawnSeed
   │                            │
같은 난수 스트림              같은 난수 스트림
라인 트레이스                 라인 트레이스
   ↓                            ↓
밥알 500개 (로컬 스폰)        밥알 500개 (로컬 스폰)
```

| 항목 | 네트워크를 건너는 것 |
|---|---|
| 밥알 500개의 위치 | `SpawnSeed` **int 1개** |
| 병합 한 번 | **int 3개** — `Multicast_MergeRice(TargetIndex, ConsumedIndex, NewGrainCount)` |

**실측 (2인 접속):** Out Rate `1,309 ~ 3,693 bytes/s`
밥알 500개의 트랜스폼을 최대로 압축해 보내도 스냅샷 한 번(약 10,000바이트)이 이 초당 총량을 넘는다.

배치가 일치하는지는 두 화면에서 체크섬을 찍어 확인했다 — `rice count: 200`, `seed: 899502592`.

## 설계 규칙 — 생성 → 전송 → 해석 → 보관

네 규칙이 값의 생애를 순서대로 따라간다.

1. **값을 만드는 곳은 하나로 두고, 전달은 복제에 맡긴다**
   플레이어 번호는 서버의 GameMode가 접속 순서대로 한 번 배정하고 PlayerState가 복제한다. 각 머신이 세면 머신마다 달라진다.
2. **복제되지 않는 로컬 액터는 인덱스로만 지칭한다**
   액터 레퍼런스는 머신 간에 해석되지 않기 때문이다.
3. **인덱스를 해석하는 책임은 그 배열을 소유한 액터에만 둔다**
   `GetRiceByIndex`는 `AllRice`를 가진 `BP_GameController_Rice`에만 있다.
4. **월드 단위 공유 참조는 GameState가 소유한다**
   `Get All Actors Of Class` 중복 탐색을 없앤다.

## 밥알 뭉치의 모양은 수식이 결정한다

저장된 좌표가 없어야 각 머신이 같은 뭉치를 만들 수 있다.

```
축마다 독립인 무리수    0.8191725 · 0.6710436 · 0.5497005
단면 프로파일          Pow(1 - Pow(p, 4), 0.25)
```

뭉치는 액터 N개가 아니라 **Instanced Static Mesh 하나**다. `GrainCount`가 바뀔 때만 인스턴스를 다시 배치하므로 뭉치가 몇 알이든 드로우콜은 하나다.

## 점수 — 200알 뭉치는 100알 뭉치보다 점수가 낮다

```
Q     = Clamp(1 - |1 - 알수/100| × 0.9,  0.1,  1)
Score = 알수 × 10 × Q
```

| 납품 방식 | 총점 |
|---|---|
| 100알 뭉치 4개 | **4,000** |
| 200알 뭉치 2개 | **400** |

- 뭉치를 구성하는 밥알 수에 정비례하면 → 그저 크게 뭉치기 게임이 된다
- 납품 횟수에만 비례하면 → 뭉칠 이유가 사라진다
- 밥알 수에 완성도를 곱하면 → **밥알당 점수가 100알에서 최대가 된다**

## 게이지가 꽉 차는 것은 100%가 아니다

100알(완성도 100%)이 바의 **80% 지점**에 오고, 그 위로는 붉어지며 계속 차오른다.

눈금도, 숫자도 넣지 않았다. 수치가 있으면 게이지는 **상태에서 목표로 바뀌고**, 판단이 "선까지 채운다"는 단순 실행으로 내려앉는다. 색도 급전환이 아니라 그라데이션이다 — 딱 끊기면 한 판에 경계를 배워버린다.

**최적점을 여러 판에 걸쳐 알아내는 것이 이 게임이다.**

## 게임 흐름

```
L_Menu  ──[방 만들기]──▶  L_Lobby  ──[시작]──▶  Stylized_Interior
        open L_Lobby?listen        servertravel
```

어느 맵에 있느냐가 곧 어느 화면이다. 위젯 스위처도, 화면 전환용 상태도 없다.

`servertravel`은 접속을 유지한 채 전원을 이동시킨다 — 각자 맵을 열면 연결이 끊긴다. 로비의 4칸은 이미 복제되는 `GameState.PlayerArray`를 읽으므로 접속자를 알리기 위한 새 변수도 RPC도 없다.

## GAS

도구의 동작은 GameplayAbility가 맡는다. 입력이 직접 상태를 바꾸지 않고, 어빌리티 활성화가 상태를 소유한다.

- `GA_Chopsticks_Pinch` — Net Execution Policy `Local Only`
- `State.Pinching` 태그 — Activation Blocked Tags로 활성화 중 재진입을 막는다
- `GE_PinchCooldown` — UE 5.6에는 인라인 `Granted Tags`가 없어 **Target Tags Gameplay Effect Component**로 태그를 부여한다
- ASC는 캐릭터에 부착해 BeginPlay 한 번으로 서버·클라 양쪽이 초기화된다

## 실행 방법

**에디터에서 2인**

1. `Content/ThirdPerson/L_Menu` 를 열고 플레이
2. `[방 만들기]` — 리슨 서버로 로비가 열린다
3. 두 번째 창에서 `[방 참가]` → `127.0.0.1`
4. 호스트가 `[시작]` → 두 화면이 함께 주방으로 이동한다

**패키지 빌드에서**

```
Build Configuration    Development
쿡할 맵                L_Menu · L_Lobby · Stylized_Interior
```

```powershell
# 로비를 건너뛰고 바로 주방을 열 때
Cooked.exe Stylized_Interior?listen -log -windowed -resx=1280 -resy=720
Cooked.exe 192.168.0.15            -log -windowed -resx=1280 -resy=720
```

같은 공유기라면 호스트의 내부 IP, 다른 장소라면 공인 IP를 입력한다. 원격은 호스트 쪽 공유기에 **UDP 7777** 포트포워딩이 필요하다.

## 검증한 범위

- 2인 PIE — 서버·클라이언트 분리, 로비에서 주방까지 전체 흐름
- 두 화면의 밥알 배치 체크섬 일치
- `stat net` Out Rate · `stat unit` 프레임 시간
- **서로 다른 네트워크의 PC 두 대에서 원격 2인 플레이** — 약 110km 떨어진 두 지점, 호스트 쪽 공유기에 UDP 7777 포트포워딩

왕복 지연은 측정하지 않았다.

## TODO

| 현재 빌드의 한계 | 판단 | 해결 방향 |
|---|---|---|
| 늦은 접속자 미지원 | 로비에서 모여 시작하는 것이 전제라, 라운드 중 합류가 흐름에 없다 | `ConsumedIndices` 복제로 접속 시 병합 상태 재생 |
| GAS가 블루프린트 전용 | 어트리뷰트가 필요한 기능(스태미나·코스트)을 게임 설계에 넣지 않았다 | 스태미나 도입 시 `UAttributeSet`과 Cost GE를 C++로 작성 |
| 세션 검색 없음 | 세션 검색은 온라인 서브시스템 설정이 필요해 직접 IP 접속으로 대체했다 | `OnlineSubsystem` 기반 LAN 세션 검색 · 입력창을 목록으로 교체 |
| 재료와 도구가 각각 한 종류 | 하나의 동기화 문제를 끝까지 미는 것을 범위로 잡았다 | `BPI_Carryable`로 재료 일반화 · 도구 거치대와 교체 기능 추가 |
| 라운드 종료 화면 없음 | 점수는 납품 시점에 확정되므로, 라운드 경계는 표시의 문제로 남았다 | GameState의 `bRoundOver` 복제 + HUD 결과 패널 |
| 사운드 팩 미커밋 | 쓰는 곡 하나만 남기고 정리해야 한다 | 정리 후 커밋 — 그전까지 클론하면 BGM이 빠진다 |

## 구조

```
Content/
  ThirdPerson/
    L_Menu.umap · L_Lobby.umap
    Blueprints/
      ManageGame/        GameMode 3 · PlayerController 3 · GameState · PlayerState · GameInstance
      UI/                WBP_Menu · WBP_Lobby · WBP_HUD
  Fab/Potatoes/potatoes_raw/
      BP_GameController_Rice   AllRice · SpawnSeed · 병합 멀티캐스트
      BP_Item_Rice             복제하지 않는다 · GrainISM
  Fab/CC0_-_Toothpick/
      BP_ToolBase · BP_Chopsticks
  StylizedKitchen/Levels/Stylized_Interior.umap
Source/Cooked/                 CookedCharacter · CookedPlayerController
```

작업 브랜치는 `feature-item-rice`이고, 페이즈 1 시점은 태그 `phase-1`이다.
커밋 메시지에 각 결정의 이유를 남겨두었다.
