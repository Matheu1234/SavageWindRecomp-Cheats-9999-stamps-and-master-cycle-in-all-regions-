# Savage Wind — diagnóstico da capacidade dos stamps (v5)

## O que a versão faz

Esta é uma atualização de **instrumentação**, sobre a versão v4 com o cheat **9999 Map Stamps Limit**. Não aumenta a capacidade serializada do save nem remove outras restrições da interface.

O diagnóstico é gerado **dentro da swgame.dll** durante a conversão de ARM64 para C. Ele usa somente BOTW Switch 1.9.0, Build ID `cd57b23fa4bbad65`, e valida os opcodes originais antes de instrumentar.

1. `0x7000D844E0` — registra a passagem pela instrução (`reached=1`), o retorno real de `rt_sw_stamps_9999_enabled()` (`enabled_return`) e o valor escolhido (`chosen=100` ou `9999`).
2. `0x7000D844F4` — reproduz **uma vez** a instrução original `STR X8, [X19,#0x228]`, relê com acesso `volatile` os dois inteiros armazenados e registra `observed_stamps`, `observed_pins` e `matches`.

O programa cria **`stamp-capacity-v5.log` na pasta da instalação** assim que carrega a DLL, mesmo que o inicializador não seja executado. Não é necessário ativar `SAVAGEWIND_STAMP_GUEST_TRACE`. O diagnóstico v5 está ligado por padrão; `SAVAGEWIND_STAMP_CAPACITY_TRACE=0` o desativa.

## Instalação incremental (MSYS2 UCRT64)

A partir de `C:\msys64\home\mr457\Stamps` com o ZIP da v5 em Downloads:

```bash
cd /home/mr457/Stamps
unzip -o '/c/Users/mr457/Downloads/SavageWind_Stamp_Capacity_v5_Incremental.zip' -d .
bash tools/prepare_stamp_capacity_v5.sh
make test-stamp-capacity-v5
make gen-helper
make dist
```

**Execute cada etapa apenas se a anterior tiver terminado sem erro.** O script de preparação normaliza as datas dos arquivos Python de `tools/` para evitar o falso erro de `clock skew` com arquivos extraídos de ZIPs. Se `make dist` falhar, não prossiga à atualização do executável.

## Atualizar a instalação existente (PowerShell)

Feche o jogo e o configurador. Em PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\msys64\home\mr457\Stamps\tools\update_existing_install.ps1" `
  -ReleaseDir "C:\msys64\home\mr457\Stamps\build\dist\Savage Wind" `
  -InstallDir "C:\Users\mr457\Downloads\Save wii u\Savage Wind"
```

**É essencial reconstruir `extracted/swgame.dll` após atualizar a instalação**, pelo configurador em **Settings → Prelaunch → Rebuild compiled game code** (ou `Set up game files` usando os ExeFS já copiados). O atualizador preserva intencionalmente a DLL existente, o save e o ExeFS do usuário. Uma atualização bem-sucedida do executável **não** incorpora os hooks na DLL anterior.

## Teste no jogo

1. Faça cópia de segurança da pasta `save`.
2. Ative **9999 Map Stamps Limit** em **Settings → Cheats**.
3. Feche o jogo. Remova **apenas** `stamp-capacity-v5.log` antigo da instalação para evitar confusão.
4. Inicie o jogo, carregue o save, abra o mapa, tente adicionar o 101º stamp (sem avançar além de 200 marcadores totais), saia normalmente.
5. Envie **`stamp-capacity-v5.log`** e **`savagewind.log`**.

## Interpretação dos registros

```text
[stamp-capacity] v5 armed; swgame.dll loaded; choice=0x7000d844e0 store=0x7000d844f4; awaiting initializer execution
[stamp-capacity] seq=1 stage=choice pc=0x7000d844e0 reached=1 enabled_return=1 chosen=9999 owner=0x... lr=0x...
[stamp-capacity] seq=2 stage=store pc=0x7000d844f4 stored_addr=0x... written_x8=0x50000270f expected_stamps=9999 observed_stamps=9999 expected_pins=5 observed_pins=5 matches=1 lr=0x...
```

- **Não existe o arquivo:** DLL v5 não carregada, escrita bloqueada ou variável de ambiente desativada. Confirme também o `savagewind.log`.
- **Só há `v5 armed`:** DLL instrumentada carregou, mas não atingiu o inicializador monitorado durante essa execução.
- **`enabled_return=0`:** o runtime informou cheat desligado no instante da escolha (independentemente do texto do launcher).
- **`chosen=9999` e `observed_stamps=9999` com `matches=1`:** o campo foi escrito corretamente naquele instante. Se ainda não aceitar o 101º stamp, a restrição está em outra etapa ou o campo é alterado depois.
- **`matches=0`:** a leitura do campo difere da escrita esperada; investigar imediatamente, sem continuar a testar o save.

O diagnóstico registra **o valor imediatamente após a gravação**, e não prova que ele persiste depois nem que o jogo use esse campo na lógica de inserção. A capacidade do save de 200 entradas continua inalterada. O cheat da motocicleta não foi modificado.

## Validação feita fora do Windows

- Teste com o NSO `main` real fornecido pelo usuário: Build ID e instruções em ambos os endereços confirmados.
- Compilação de harness C com `clang` no Linux: valores `100/5` e `9999/5` gravados e relidos, ambos `matches=1`, `NZCV` e registradores preservados nos trechos instrumentados.
- Testes do cheat v4, catálogo e probes v3 passaram.

A compilação **Windows/MSYS2 e execução real do jogo não foram verificadas neste ambiente** e dependem do procedimento acima.
