# Savage Wind — Stamp Follow-up v6

## Objetivo

O log **v5 confirmou que a capacidade de 9.999 é gravada corretamente** no inicializador ARM64 `0x7000D844F4`. A versão v6 NÃO altera o limite, o arquivo de save nem o cheat da motocicleta. É uma investigação somente de leitura para descobrir se essa capacidade persiste enquanto o mapa é utilizado e quais funções são acionadas nas tentativas de inserir stamps.

### Instrumentação adicionada

- Registra `stamp-followup-v6.log` pela **swgame.dll** (e mantém `stamp-capacity-v5.log`).
- A cada ponto conhecido de interface `0x7000DD8208`, `0x7000DD86B4`, `0x7000DF6174`, `0x7000DF618C`, grava o valor lido do mesmo campo `+0x228` observado na v5, o campo de pins `+0x22C`, PC, LR, alguns registradores e NZCV. Os rótulos da interface são *candidatos* baseados em referências a strings; não provam que cada evento seja uma ação do jogador.
- Após o primeiro `open_stamp_menu`, registra amostras limitadas das entradas de função ARM64 na região do menu `0x7000DD7000`–`0x7000DFFFFF` e, como contexto, de `0x7000D80000`–`0x7000D8FFFF` (inicialização de categorias). Todos os registros incluem PC, LR, registradores e o mesmo snapshot de capacidade.
- **Não modifica** instruções do jogo além do cheat optativo v4 já existente. No Windows, `VirtualQuery` verifica a região e `ReadProcessMemory` efetua a leitura, sem escrever em memória; se o objeto for desalocado, o snapshot é marcado como indisponível. A leitura não é garantida quando o objeto foi desalocado ou reutilizado; por isso há o campo `readable`. Um resultado `readable=1` prova a legibilidade daquele endereço no instante, não a identidade permanente da estrutura.
- Para evitar logs enormes: amostragem esparsa de chamadas (1, 2, 3, 4, 8, 16, 32, 64, 99, 100, 101, 128, 256, ...) com máximo de **64 amostras por função** e **12.000 no total**. Portanto, a ausência de uma função não demonstra que ela nunca executou.
- `SAVAGEWIND_STAMP_FOLLOWUP_TRACE=0` desliga a instrumentação. Por padrão, ela fica ativada nesta versão de diagnóstico.

## Instalação incremental

1. Coloque `SavageWind_Stamp_Followup_v6_Incremental.zip` em Downloads.
2. No **MSYS2 UCRT64**:

```bash
cd /home/mr457/Stamps
unzip -o "/c/Users/mr457/Downloads/SavageWind_Stamp_Followup_v6_Incremental.zip" -d .
bash tools/prepare_stamp_followup_v6.sh
make test-stamp-followup-v6
make gen-helper
make dist
```

3. **Somente após `make dist` concluir sem erro**, feche o jogo e o configurador e execute no PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File "C:\msys64\home\mr457\Stamps\tools\update_existing_install.ps1" `
  -ReleaseDir "C:\msys64\home\mr457\Stamps\build\dist\Savage Wind" `
  -InstallDir "C:\Users\mr457\Downloads\Save wii u\Savage Wind"
```

4. Abra **Savage Wind Configuration.exe → Settings → Prelaunch → Rebuild compiled game code**. É obrigatória uma nova `swgame.dll`, pois o atualizador preserva `extracted` e a DLL antiga.

## Teste recomendado

Faça backup da pasta `save`. Mantenha habilitado **9999 Map Stamps Limit** e não ultrapasse 200 marcadores totais.

- **Sessão A (referência):** abra o jogo, carregue o save, abra o mapa e o menu de stamps, não coloque nem remova stamps, feche o jogo normalmente; salve o arquivo `stamp-followup-v6.log` como `stamps-v6-baseline.log`.
- **Sessão B (tentativa):** execute novamente o jogo, tente adicionar o **101º stamp**, não remova nenhum; encerre normalmente e guarde `stamp-followup-v6.log` como `stamps-v6-tentativa.log`.
- Envie os dois arquivos e o `stamp-capacity-v5.log` da segunda sessão. Se o jogo travar, envie também `savagewind.log`.

A DLL cria um log novo a cada início de jogo. **Renomeie o arquivo da sessão A antes de iniciar B**, para que ele não seja substituído.

Análise opcional de uma sessão:

```bash
cd /home/mr457/Stamps
python3 tools/analyze_stamp_followup_v6.py "/c/Users/mr457/Downloads/Save wii u/Savage Wind/stamp-followup-v6.log"
```

## Leitura do resultado

- `stage=ui ... capacity=9999 readable=1`: a capacidade continua em 9.999 no instante observado na interface.
- `stage=ui ... capacity=100 readable=1`: a capacidade daquele endereço agora está em 100; investigar sobrescrita ou reutilização da memória antes de concluir a causa.
- `stage=fn ... pc=... lr=...`: função executada após a primeira referência à abertura do menu. O significado funcional ainda depende de correlacionar as duas sessões e, se necessário, desassemblar os candidatos.
- `readable=0`: não foi possível ler com segurança o endereço rastreado; não interpretar zeros de capacidade como valores reais.

**Limitação:** a v6 não identifica automaticamente a instrução exata da condição que bloqueia o 101º stamp. É uma sondagem de persistência e de caminho de execução; as diferenças entre sessões orientarão a instrumentação específica da condição, sem adivinhar endereços.

**Validação local:** testes automatizados e harness C/Linux, incluindo o `main` NSO real da versão Switch 1.9.0. Compilação Windows e execução real do jogo precisam ser verificadas na instalação do usuário.
