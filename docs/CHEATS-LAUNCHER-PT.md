# Savage Wind — catálogo de cheats (experimental)

Base: **Savage Wind 0.4.1-alpha.4**, com alterações locais.
Este ZIP é o **código-fonte**, não um executável final compilado.

## O que está implementado

- Aba **Cheats** no launcher: `Savage Wind Configuration.exe` → **SETTINGS** → **Cheats**.
- Descoberta de manifests na pasta `cheats/Nome do Cheat/cheat.ini`.
- Ativar/desativar individualmente cheats que possuam um **handler compilado e auditado**.
- Estado salvo em `savagewind-cheats.ini`, na pasta de dados do Savage Wind. **Mudanças exigem reiniciar o jogo**.
- Botão **Rebuild compiled game code** em **Settings → Prelaunch**, usando os arquivos `extracted/exefs` e `extracted/romfs` já instalados, sem reinstalar 15 GB ou apagar saves.
- `make dist` inclui os manifests no pacote Windows.

## O que NÃO está implementado

- **9999 Map Stamps Limit:** disponível como cheat **experimental e opt-in**; aumenta o limite de capacidade em RAM de 100 para 9999, mas o save continua limitado a 200 marcadores. Faça backup antes de habilitar.
- Cheats arbitrários PowerPC/ARM64, patches IPS/PCHTXT e execução de DLLs externas. A presença de um `cheat.ini` não é autorização para executar código arbitrário. **Somente os handlers `motorcycle_climate_mask` e `stamp_capacity_9999` para o BOTW Switch build `cd57b23fa4bbad65` estão registrados**.
- O cheat da motocicleta ainda é **experimental**; os testes de jogabilidade em Gerudo e Eldin não foram realizados.

## Estrutura dos cheats

```text
Savage Wind/
├─ savagewind.exe
├─ Savage Wind Configuration.exe
├─ savagewind-cheats.ini               # gerado quando alternar uma opção
├─ cheats/
│  ├─ Motorcycle in All Regions/
│  │  └─ cheat.ini
│  └─ 9999 Map Stamps Limit/
│     └─ cheat.ini                     # ativo como experimental quando marcado
├─ extracted/                          # arquivos do jogo: PRESERVAR
├─ save/                               # saves: PRESERVAR
└─ savagewind.ini                      # configurações: PRESERVAR
```

Manifest `cheat.ini` reconhece `id`, `name`, `description`, `handler`, `build_id`, `status`.
Para adicionar um novo cheat funcional, **não basta criar o manifest**: é necessário implementar e testar um handler no código do Savage Wind, registrá-lo em `supported_hook()` e, se necessário, adicionar pontos de patch em `tools/patches.py`. O launcher exibirá manifests sem handler como indisponíveis.

## Compilar no Windows

Instale os requisitos conforme `docs/BUILD-windows.md`:

```powershell
powershell -ExecutionPolicy Bypass -File tools\setup_windows.ps1
```

No **terminal MSYS2 UCRT64**, dentro da pasta extraída deste projeto:

```bash
export PATH="/c/msys64/ucrt64/bin:/c/msys64/usr/bin:$PATH"
make -j runtime-objs
```

Para construir **uma distribuição portátil atualizável** (requer tempo, dependências e ferramentas do projeto):

```bash
make deps-windows
make gen-helper
tools/fetch_zig.sh
make dist
```

O resultado fica em `build/dist/SavageWind-0.4.1-alpha.4-windows.zip` (o nome pode variar conforme `VERSION`). **Esta compilação final de Windows não foi executada neste ambiente Linux.**

## Atualizar sua instalação atual sem perder saves

1. Feche completamente o Savage Wind. Faça um backup da instalação atual.
2. Compile a nova distribuição no Windows usando `make dist`. Extraia seu conteúdo `Savage Wind/` para uma pasta temporária.
3. No PowerShell, a partir da pasta deste projeto, execute (ajuste os caminhos):

```powershell
powershell -ExecutionPolicy Bypass -File tools\update_existing_install.ps1 `
  -ReleaseDir "C:\Temp\Savage Wind" `
  -InstallDir "C:\Jogos\Savage Wind"
```

4. Abra `Savage Wind Configuration.exe` da pasta atualizada. Caso apareça a tela de configuração inicial porque a DLL antiga é incompatível, clique em **Set up game files** para reconstruir a biblioteca usando o `extracted/` já presente.
5. Após a configuração, também existe o comando **Settings → Prelaunch → Rebuild compiled game code**, para refazer a recompilação manualmente. Isso recompila `swgame.dll` a partir do `extracted/` existente, com o hook da motocicleta integrado.
6. Selecione **Settings → Cheats → Motorcycle in All Regions** e ative a caixa. O cheat passa a valer na **próxima inicialização**.
7. Faça os testes com **um save de cópia**, idealmente em Gerudo, Eldin e uma região comum. Sem a Master Cycle Zero desbloqueada, o cheat não força seu desbloqueio.

O script não sobrescreve `extracted`, `save`, `cache`, `cheats`, `savagewind.ini` nem `savagewind-cheats.ini`. Ele faz backup dos programas/arquivos substituídos em `.savagewind-update-backup-*`. Não execute o atualizador com o Savage Wind aberto.

**Limitação:** se a recompilação do game code falhar, o mecanismo de build original pode remover uma DLL previamente recompilada; por isso também mantenha um backup de `extracted/swgame.dll` e `extracted/swgame.dll.id` antes de começar. Os saves não são afetados.

## Testes de desenvolvimento (Linux/macOS)

```bash
clang -std=c11 -Wall -Wextra -Werror -Iruntime \
  tests/test_sw_cheats.c runtime/sw_cheats.c -o /tmp/test_sw_cheats
/tmp/test_sw_cheats
clang -std=c11 -Wall -Wextra -Werror -Iruntime \
  tests/test_sw_cheat_catalog.c runtime/sw_cheats.c -o /tmp/test_catalog
/tmp/test_catalog
python3 -m unittest discover -s tests -p 'test_motorcycle*.py'
```

Os testes acima são **isolados**. Não demonstram que o jogo recompilado ou o launcher Windows funcionam até serem compilados e executados no Windows.


## 9999 Map Stamps Limit (experimental, v4)

O handler está implementado no inicializador de capacidade ARM64 `0x7000D844E0`.
A nova opção deve ser ligada em **Settings → Cheats** após reconstruir a `swgame.dll`.
**O save continua limitado a 200 marcadores totais; o uso acima disso pode perder dados.**
Consulte [STAMPS-CHEAT-V4-PT.md](STAMPS-CHEAT-V4-PT.md) para limites, testes e instalação.
