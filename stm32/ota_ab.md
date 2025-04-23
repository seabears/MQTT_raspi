

# STM32 부트로더 구현: Partition A/B로 조건부 점프

STM32에서 두 개의 애플리케이션 파티션(Partition A, Partition B)을 선택적으로 실행할 수 있는 간단한 부트로더 코드를 구현했습니다. 해당 코드는 백업 레지스터를 통해 부팅할 파티션을 결정하고, LED 점멸로 선택된 파티션을 표시한 후 해당 애플리케이션으로 점프합니다.

---

## 📁 프로젝트 개요

- **MCU**: STM32 (예: STM32F103 계열)
- **목표**: 부트로더가 Partition A 또는 Partition B 중 하나를 선택해서 실행
- **부팅 상태 저장**: Backup 레지스터 (BKP->DR1)
- **LED 표시**: GPIO로 LD2 핀 점멸

---

## 🧠 주요 개념

| 항목 | 설명 |
|------|------|
| Partition A 주소 | `0x08008000` |
| Partition B 주소 | `0x08014000` |
| 부팅 플래그 주소 | `BKP->DR1` |
| 플래그 값 | `0x01` (A), `0x02` (B) |

---

## 🛠 코드 설명

### 1. 부팅 플래그 확인

```c
uint32_t boot_flag = BOOT_FLAG_ADDR;
if (boot_flag != BOOT_FLAG_A && boot_flag != BOOT_FLAG_B){
    boot_flag = BOOT_FLAG_A;
    BOOT_FLAG_ADDR = boot_flag;
}
```

> 잘못된 값일 경우 기본적으로 Partition A로 설정

---

### 2. 파티션에 따른 LED 점멸 및 점프 주소 설정

```c
if (boot_flag == BOOT_FLAG_A){
    appAddress = APPLICATION_A_ADDRESS;
    // LD2 LED 1회 점멸
} else if (boot_flag == BOOT_FLAG_B){
    appAddress = APPLICATION_B_ADDRESS;
    // LD2 LED 2회 점멸
}
```

> 선택된 파티션을 시각적으로 구분 가능하게 LED 점멸

---

### 3. 애플리케이션으로 점프하는 함수

```c
void JumpToApplication(uint32_t appAddress){
    uint32_t appStack = *(__IO uint32_t*)appAddress;
    uint32_t appEntry = *(__IO uint32_t*)(appAddress + 4);
    pFunction appResetHandler = (pFunction)appEntry;

    __disable_irq();
    HAL_DeInit();
    __set_MSP(appStack);
    SCB->VTOR = appAddress;
    __enable_irq();
    appResetHandler();
}
```

> 해당 애플리케이션의 스택 및 엔트리 주소를 설정하고 실행

---

### 4. 백업 도메인 설정 함수

```c
void ConfigureBackupDomain(void){
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {}

    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
}
```

> BKP 레지스터 접근을 위한 권한 활성화 및 설정

---

## 📝 주의 사항

- 파티션 A와 B의 시작 주소는 반드시 각각의 앱에서 `Vector Table` 기준으로 설정되어야 함.
- STM32의 플래시 맵을 정확히 이해하고 파티션 주소를 설정해야 함.
- 보안 또는 무결성이 중요한 경우 CRC 또는 signature 체크 로직 추가 필요.

---

## ✅ 결과

- Partition A → LD2 1회 점멸 후 앱 실행  
- Partition B → LD2 2회 점멸 후 앱 실행  
- 백업 레지스터 값을 조정하면 다음 부팅 시 앱 선택 가능

---

## 🔚 마무리

이 구조는 **이중 펌웨어 시스템**, **펌웨어 OTA 업데이트**, 또는 **롤백 시스템** 구현에 매우 유용합니다. 이후 개선으로는 UART/SPI를 통해 부팅 플래그를 제어하거나, 부팅 후 검증 실패 시 자동 롤백 기능 등을 구현할 수 있습니다.

--- 

필요에 따라 GIF나 LED 점멸 동작을 영상으로 첨부하면 더욱 보기 좋습니다.  
혹시 OTA나 부트 플래그 변경 방법도 추가 설명 원하시나요?