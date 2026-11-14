# Savage Wind — Diagnóstico de operações com stamps (v2)

**Estado:** implementado e testado no código do runtime em ambiente de teste local. **Não testado em uma sessão real do BOTW/Windows** neste pacote. A opção do cheat **9999 Map Stamps Limit permanece desabilitada**. Nenhum limite do jogo é modificado.

## Por que a v1 não bastava

Os logs A/B de novembro de 2026 exibiram os mesmos 70 endereços monitorados; as comparações ARM64 com `99` e `100` são muito frequentes e não indicam necessariamente uma operação com stamps. A v2 coleta **entradas de funções recompiladas** em janelas de tempo explicitamente rotuladas `baseline`, `add` e `remove`, permitindo contrastar o conjunto de funções e seus chamadores em vez de inferir semântica pelo número `100`.

## Como funciona

- Aproveita os pontos `FN_ENTER()` **já compilados** em cada função gerada. Não altera `tools/gen.py`, `tools/patches.py`, `runtime/cpu.h` nem o identificador da `swgame.dll`.
- Com `SAVAGEWIND_STAMP_FLOW=1`, um arquivo local `stamp-flow-control.txt` permite iniciar/encerrar uma fase sem recompilar ou reiniciar o jogo.
- A cada 15 quadros apresentados, o runtime lê `stamp-flow-control.txt`. Só aceita `baseline`, `add`, `remove` e `idle` (valores exatos). Se estiver ausente, usa `idle`.
- Durante uma fase, registra a quantidade de entradas de cada **par (função ARM64 recompilada, endereço LR do chamador)**, mais uma amostra dos registradores de entrada `x0..x5`. Não faz `dereference` desses ponteiros e não consulta nem altera dados do jogo.
- Ao trocar de fase, salva um arquivo `stamp-flow-<época>-<sequência>-<fase>.tsv` na pasta de dados do jogo. As gravações são feitas **depois** de desativar a captura e esvaziar as chamadas já iniciadas.
- Sem `SAVAGEWIND_STAMP_FLOW=1`, não há leitura periódica do arquivo nem coleta de entradas. Não use simultaneamente `SAVAGEWIND_FNPROF` (a inicialização desativa a v2 se estiver configurado).
- A v1 continua independente: `SAVAGEWIND_STAMP_TRACE=1` registra comparações genéricas, mas não é necessário no teste v2.

O endereço LR nem sempre identifica o chamador exato em uma chamada de cauda; os registradores em cada TSV são **observações**, não contagens comprovadas de stamps.

## Compilação e atualização (Windows, MSYS2 UCRT64)

Na pasta raiz de **seu projeto já compilado**, aplique o patch ou sobreponha os arquivos do pacote `SavageWind-stamp-flow-v2-overlay.zip` (o diretório raiz do ZIP corresponde à raiz de `Stamps`). Preserve os três arquivos `.ttf` que você recuperou anteriormente. Em MSYS2 UCRT64, após copiar os arquivos, execute o script que garante a recompilação dos objetos alterados:

```bash
bash tools/prepare_stamp_flow_v2.sh
make test-stamp-flow
make test-stamp-trace
make dist
```

Em PowerShell, com o Savage Wind fechado, atualize a instalação existente usando o script do próprio projeto:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\msys64\home\mr457\Stamps\tools\update_existing_install.ps1" `
  -ReleaseDir "C:\msys64\home\mr457\Stamps\build\dist\Savage Wind" `
  -InstallDir "C:\Users\mr457\Downloads\Save wii u\Savage Wind"
```

**Não é preciso fazer `Rebuild compiled game code` para a v2**, pois o `FN_ENTER()` já faz parte da DLL compilada na versão anterior. Isso só vale se a sua `swgame.dll` atual estiver funcional e for compatível com este código-base; não remova os arquivos de `extracted/`.

## Captura das três fases

Faça uma cópia de segurança dos saves por precaução, já que o próprio jogo poderá salvar alterações de marcadores normalmente. O diagnóstico não cria ou modifica saves.

**Janela 1 — PowerShell:**

```powershell
$game = 'C:\Users\mr457\Downloads\Save wii u\Savage Wind'
Remove-Item (Join-Path $game 'stamp-flow-control.txt') -ErrorAction SilentlyContinue
Remove-Item Env:SAVAGEWIND_FNPROF -ErrorAction SilentlyContinue
Remove-Item Env:SAVAGEWIND_STAMP_TRACE -ErrorAction SilentlyContinue
$env:SAVAGEWIND_STAMP_FLOW = '1'
& (Join-Path $game 'savagewind.exe')
```

O log deverá registrar:

```text
[stamp-flow] enabled; control=stamp-flow-control.txt; commands=baseline,add,remove,idle; no gameplay changes
```

**Janela 2 — PowerShell (mantenha aberta):**

```powershell
$tool = 'C:\msys64\home\mr457\Stamps\tools\stamp_flow_control.ps1'
$game = 'C:\Users\mr457\Downloads\Save wii u\Savage Wind'

# A: com o mapa aberto, navegue no menu sem inserir/remover stamps;
# deixe o jogo ativo por alguns segundos.
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase baseline
# ... volte ao jogo para permitir a coleta, depois retorne ao PowerShell:
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase idle

# B: abra o mesmo menu. Ative 'add', volte ao jogo, adicione UM stamp.
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase add
# ... faça a operação, volte ao PowerShell:
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase idle

# C: ative 'remove', volte ao jogo, remova UM stamp.
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase remove
# ... faça a operação, volte ao PowerShell:
powershell -ExecutionPolicy Bypass -File $tool -InstallDir $game -Phase idle
```

**Atenção:** `baseline`, `add` e `remove` são etiquetas **manuais**. A captura não sabe automaticamente se você inseriu um marcador. Mantenha cada fase curta. **Após escrever `idle`, retorne ao jogo por pelo menos um segundo** para que a parada seja percebida e o relatório seja gravado **antes** de iniciar a próxima fase. A consulta acontece a cada 15 quadros apresentados; se `idle` for sobrescrito imediatamente, essa transição poderá ser perdida.

O log indicará as transições:

```text
[stamp-flow] begin phase=add frame=...
[stamp-flow] end phase=add edges=... calls=... dropped=0 output=stamp-flow-...-add.tsv
```

Se `dropped` não for zero, parte dos pares função/chamador excedeu a capacidade de 262.144 entradas e a amostra está incompleta.

## Analisar e enviar os relatórios

No MSYS2 UCRT64, execute passando os caminhos reais dos três arquivos gerados (pode usar aspas em nomes com espaços):

```bash
python3 tools/analyze_stamp_flow.py \
  '/c/Users/mr457/Downloads/Save wii u/Savage Wind/stamp-flow-...-baseline.tsv' \
  '/c/Users/mr457/Downloads/Save wii u/Savage Wind/stamp-flow-...-add.tsv' \
  '/c/Users/mr457/Downloads/Save wii u/Savage Wind/stamp-flow-...-remove.tsv' \
  --top 40
```

Envie os **três `.tsv`** e, se possível, o novo `savagewind.log`. O script apresenta funções exclusivas de cada fase, frequência relativa e possíveis chamadores. A próxima etapa será validar os candidatos com tracing de argumentos/retornos e checar o armazenamento dos marcadores e serialização dos saves antes de habilitar o patch.

## Testes locais realizados

`make test-stamp-flow` compila um pequeno programa C com CPU simulada e verifica transições de fase, criação de TSV, contadores, endereço do chamador e captura de argumentos. `make test-stamp-trace` mantém os testes da v1. Os testes **não garantem** o funcionamento com BOTW em Windows; é necessária validação em campo.
