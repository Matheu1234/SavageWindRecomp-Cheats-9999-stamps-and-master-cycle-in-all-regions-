> **Histórico da fase de diagnóstico (v1/v2).** O cheat experimental foi implementado na versão v4; consulte [STAMPS-CHEAT-V4-PT.md](STAMPS-CHEAT-V4-PT.md).

# BOTW Switch 1.9.0 — 9999 Map Stamps Limit (em investigação)

**Versão:** `Savage Wind 0.4.1-alpha.4`, Build ID `cd57b23fa4bbad65`.

## Situação atual

Esta entrega **não habilita o cheat de 9.999**. Ela instrumenta, sem alterar os
valores ou flags da CPU do jogo, 203 comparações ARM64 com os imediatos 99, 100
ou 199, seis construções `MOV W0,#100` e quatro referências diretas aos
componentes `mc_openStampMenu`, `Pa_StampNum_00` e `Pa_StampBox_00`.

O candidato `0x7000d91408` (categoria 4, `CMP W8,#100`) foi confirmado como
uma comparação legítima; porém seu chamador também limita quantidades de
registros a **999** e manipula categorias com máximos 40, 60 e 160, o que torna
**plausível** que seja gerência de inventário em vez do mapa. Logo, **não**
foi usado como alvo do cheat.

Na versão Wii U, a documentação do Cemu alerta que *pins + stamps* não devem
ultrapassar **200**, mesmo com o graphic pack 9999; caso contrário podem sumir.
Ver: https://wiki.cemu.info/wiki/ALZP01 . Esse número não comprova a mesma
capacidade no Switch; temos de verificar arrays e serialização do save.

## Como testar os diagnósticos no Windows

1. Compile `make dist` em MSYS2 UCRT64, como na versão anterior.
2. Atualize os binários do Savage Wind com o script
   `tools/update_existing_install.ps1` e execute **Rebuild compiled game code**.
   Isso recompila a `swgame.dll` com os probes (apenas do Build ID acima).
3. Execute **PowerShell** na pasta de dados do Savage Wind, onde fica
   `savagewind.ini`, ou defina a variável antes de iniciar o EXE:

```powershell
$env:SAVAGEWIND_STAMP_TRACE = '1'
& 'C:\Jogos\Savage Wind\savagewind.exe'
Remove-Item Env:SAVAGEWIND_STAMP_TRACE
```

**Atenção:** ajuste o caminho do EXE. O comando deve iniciar uma nova instância
no mesmo PowerShell. O log registra `[stamp-trace] enabled` quando está ativo.

4. Inicie **sessão A**: abra o mapa, abra/feche o menu de stamps e feche o jogo.
5. Inicie **sessão B**: abra o mapa, adicione/remova alguns stamps, feche o jogo.
   Capture cada `savagewind.log` separadamente. Se possível, faça uma sessão C
   adicionando um marcador ao chegar ao limite de 100 (com save de teste).
6. Compare os logs com:

```bash
python tools/summarize_stamp_trace.py A.log B.log
```

As instruções `CMP` têm sua semântica e `NZCV` originais mantidas; a única
mudança é registrar eventos no log quando a variável estiver ativa. A
comparação usa `CSET` e continua sendo executada pelo código recompilado.

## O que falta para a implementação funcional

- Relacionar uma condição real de 100 stamps ao caminho de adição de marcador.
- Rastrear as chamadas indiretas do menu aos componentes de dados.
- Verificar o armazenamento de stamps e pins e os limites de serialização.
- Só então criar um handler para alterar o máximo. `9999` excede o imediato
  AArch64 de 12 bits de um `CMP` comum, portanto não basta trocar um opcode.
- Rodar teste de save->reiniciar->carregar; checar limites e desativação.

**Não distribua arquivos do jogo em repositórios públicos**; o projeto-fonte
não inclui nenhum NSO/ROMFS/saves do jogo.
