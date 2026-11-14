# Savage Wind — diagnóstico de stamps v3 (hooks no código recompilado)

**Compatibilidade:** BOTW Switch 1.9.0, Build ID `cd57b23fa4bbad65`.
**O cheat 9999 Map Stamps Limit permanece desabilitado.** Não há mudança no limite, na estrutura de dados ou nos saves.

## Por que refazer?

A v2 dependia do runtime e de um arquivo `stamp-flow-control.txt` para detectar fases. Esta versão **não usa o controle de fases**, não depende de `rt_sw_stamp_flow_*` e não pede três sessões distintas. O gerador acrescenta probes diretamente às instruções ARM64 que são recompiladas para C, como o primeiro diagnóstico dos `CMP #99/#100`.

As sondas instrumentam:

- Os **mesmos 203 CMP, 6 MOV e 4 endereços de referência de UI**, fixados ao Build ID e com checagem dos opcodes. Executam a instrução original, sem alterar o estado do processador emulado.
- As **entradas das funções ARM64** nas regiões de código próximas ao menu (`0x7000dc0000–0x7000e00000`) e a um conjunto de comparadores anteriores (`0x70007d0000–0x70007d5000`), somente quando o Build ID corresponde. O rastreio de funções é ativado somente depois da primeira abertura detectada do menu.
- O código de captura, limites de amostras e gravação do arquivo ficam em `tools/gamelib_entry.c`, que é compilado **para dentro da `swgame.dll`**, não no runtime principal.

A coleta anota o PC, o chamador (`LR`), alguns registradores ARM64, o valor de comparação e a **época** de abertura do menu. Registradores brutos não são contagens de stamps validadas. A captura tem limite de eventos para evitar crescimento ilimitado. Portanto, esse método amplia a observabilidade, mas **ainda não garante identificar a função do 101º stamp**.

## Instalação incremental, MSYS2 UCRT64

1. Mantenha sua cópia do projeto em `/home/mr457/Stamps` e preserve as dependências já instaladas, inclusive os arquivos de fontes tipográficas recuperados anteriormente. Faça uma cópia de segurança da pasta da instalação e dos saves.
2. Feche o Savage Wind. Extraia **`SavageWind-stamp-guest-v3-overlay.zip`** na raiz de `/home/mr457/Stamps`, sobrescrevendo os arquivos correspondentes.
3. No MSYS2 UCRT64, execute:

```bash
cd /home/mr457/Stamps
bash tools/prepare_stamp_guest_v3.sh
make test-stamp-guest
make test-stamp-trace
make gen-helper
make dist
```

**Importante:** `make gen-helper` precisa ser executado novamente porque `tools/gen.py` e `tools/patches.py` mudaram. O `make dist` usa o helper atualizado. Se aparecer novamente erro de arquivos `.ttf` ausentes, recupere-os de sua cópia legítima anterior; não substitua arquivos por nomes falsos.

4. Atualize a instalação existente, usando **PowerShell**:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\msys64\home\mr457\Stamps\tools\update_existing_install.ps1" `
  -ReleaseDir "C:\msys64\home\mr457\Stamps\build\dist\Savage Wind" `
  -InstallDir "C:\Users\mr457\Downloads\Save wii u\Savage Wind"
```

5. Abra o launcher atualizado e use **Settings → Prelaunch → Rebuild compiled game code** para **reconstruir `swgame.dll`**. Esta etapa é obrigatória na v3: os novos hooks ficam **na DLL do jogo recompilado**, não somente no executável.

## Uma sessão, sem controlador, sem TSVs

Abra uma janela PowerShell, com o jogo fechado, e execute:

```powershell
$game = 'C:\Users\mr457\Downloads\Save wii u\Savage Wind'
Remove-Item Env:SAVAGEWIND_STAMP_FLOW,Env:SAVAGEWIND_STAMP_TRACE -ErrorAction SilentlyContinue
$env:SAVAGEWIND_STAMP_GUEST_TRACE = '1'
& (Join-Path $game 'savagewind.exe')
```

No jogo: carregue um save, **abra o menu de stamps, adicione um stamp e depois remova esse stamp**, e encerre normalmente. Não precisa alternar entre PowerShell e jogo. O monitoramento inicia automaticamente ao passar pela referência `open_stamp_menu`.

Na pasta principal do Savage Wind será gerado **`stamp-guest-v3.log`**, contendo:

```
[stamp-guest] enabled v3 (swgame.dll instruction hooks; read-only)
[stamp-guest] seq=1 epoch=1 type=ui pc=0x7000dd8208 ...
[stamp-guest] seq=2 epoch=1 type=fn pc=0x... ...
[stamp-guest] seq=3 epoch=1 type=cmp pc=0x... ...
```

O `savagewind.log` pode também registrar `output=stamp-guest-v3.log`; porém o arquivo de captura detalhada é **`stamp-guest-v3.log`** para evitar problemas entre as bibliotecas C do executável Windows e da DLL recompilada.

Para um resumo local opcional:

```bash
python tools/analyze_stamp_guest_v3.py '/c/Users/mr457/Downloads/Save wii u/Savage Wind/stamp-guest-v3.log' --out stamp_guest_resumo.md
```

**Envie o arquivo `stamp-guest-v3.log`** para a análise. Caso não exista, verifique primeiro se a DLL do jogo foi reconstruída; o log principal deverá indicar pelo menos a habilitação do v3.

## Limitações e segurança

- Somente leitura: não altera comparações, flags ou valores dos registradores do jogo.
- O log contém endereços e registradores brutos, úteis para engenharia reversa, mas não necessariamente interpretáveis como stamps.
- As janelas monitoradas foram escolhidas a partir de referências já verificadas; funções fora dessas janelas ainda podem participar da inserção de marcadores.
- A instrumentação gera overhead e logs de diagnóstico; **desative** `SAVAGEWIND_STAMP_GUEST_TRACE` no uso normal.
- `9999` é maior que o imediato comum de `CMP` ARM64; antes de modificar limite é obrigatório verificar alocação, interface, pins e persistência no save.
