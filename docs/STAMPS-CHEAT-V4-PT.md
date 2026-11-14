# Savage Wind 0.4.1-alpha.4 — cheat experimental 9999 Map Stamps Limit (v4)

**Estado:** implementado no código recompilado; compilação/execução do jogo Windows depende dos arquivos locais e **não foi verificada aqui**.

## O que faz

O cheat do Cemu no `U-King.rpx` Wii U substitui `li r0, 100` em `0x02E9F2C0` por `li r0, 9999`. A inspeção do `main` NSO Switch 1.9.0, Build ID `cd57b23fa4bbad65`, identificou o inicializador equivalente:

```asm
0x7000D844E0  MOV  X8, #100             ; substituído condicionalmente por 9999
0x7000D844E4  MOVK X8, #5, LSL #32     ; mantido: cinco pins
0x7000D844F4  STR  X8, [X19, #0x228]  ; dois limites numa única gravação
0x7000D844F8  STR  W21,[X19, #0x230]  ; mantido: um marcador especial
```

O gerador em `tools/patches.py` substitui APENAS a instrução `0x7000D844E0`, depois de validar Build ID e quatro palavras de instrução. Os controles do launcher e `savagewind-cheats.ini` acionam `rt_sw_stamps_9999_enabled()` no boot do jogo. **Desativado = 100. Ativado = 9999.** Exige reinício após alternar. A v3 de diagnóstico continua opcional. O cheat da motocicleta continua independente.

## ALERTA: não altera capacidade do save

A pesquisa anterior detectou 200 entradas serializadas de marcadores `MapApp_MapIconNo/Pos`. O limite 9999 modifica a capacidade de memória, **não expande o formato do save**. Colocar mais de 200 marcadores totais pode perder dados ao salvar/recarregar e produzir efeitos imprevistos. Mesmo antes de 200, estabilidade não foi comprovada.

**Faça backup da pasta `save` e da DLL antes de testar; use um save de teste.** Para validar de modo conservador: marque o número inicial (100 stamps + pins), habilite o cheat e tente adicionar um stamp (101º), salvar, fechar e recarregar. Verifique se ele permanece. Não tente chegar a 9999 sem examinar serialização e arrays.

## Instalar sobre projeto existente no MSYS2 UCRT64

Copie os arquivos do overlay v4 para a raiz de `/home/mr457/Stamps`, sobrescrevendo os respectivos arquivos do projeto v3. Depois execute:

```bash
cd /home/mr457/Stamps
bash tools/prepare_stamp_cheat_v4.sh
make test-stamp-cheat-v4
make gen-helper
make dist
```

As dependências de compilação do Windows, fonts legítimas e Zig previamente instalados continuam necessários. `make dist` cria `build/dist/Savage Wind/` como distribuição. **Não basta compilar a pasta raiz `Stamps`: o script de atualização recebe a distribuição compilada.**

No PowerShell, com o jogo fechado:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\msys64\home\mr457\Stamps\tools\update_existing_install.ps1" `
  -ReleaseDir "C:\msys64\home\mr457\Stamps\build\dist\Savage Wind" `
  -InstallDir "C:\Users\mr457\Downloads\Save wii u\Savage Wind"
```

O script copia os binários gerados, atualiza o manifest **embutido** do cheat v3 (faz backup do anterior) e preserva saves, `extracted`, outros cheats e configurações do usuário.

No launcher `Savage Wind Configuration.exe` execute **Settings → Prelaunch → Rebuild compiled game code**. A reconstrução da `swgame.dll` é **obrigatória uma única vez** para instalar o patch ARM64.

Após rebuild: **Settings → Cheats → 9999 Map Stamps Limit → ativar**. Reinicie o jogo. O `savagewind.log` deve apresentar:

```text
[cheats] 9999 Map Stamps Limit: EXPERIMENTAL ON (guest init @ 0x7000d844e0; SAVE LIMIT NOT EXPANDED)
```

Para desfazer: desative em Cheats, reinicie o jogo; o patch passa a carregar 100 novamente. Não é preciso recompilar DLL para alternar.

## Testes e limitações

`make test-stamp-cheat-v4` verifica opcodes de proteção, controle por manifest, habilitar/desabilitar independentemente, preservação do valor 5 e recusa de outros Build IDs, usando dados artificiais. A compilação completa Windows não pode ser executada neste Linux sem as dependências da sua instalação; execução in-game e persistência após reload ainda requerem teste real. O pacote não distribui arquivos proprietários do BOTW.
