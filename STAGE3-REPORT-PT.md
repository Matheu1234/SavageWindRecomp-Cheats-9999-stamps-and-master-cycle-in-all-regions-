# Savage Wind 0.4.1-alpha.4 — etapa 3: descompilação e hook experimental

## Material verificado

- 46 partes do RAR `Breath of the wild extraido`, da part01 à part46, identificadas no ambiente.
- Catálogo RAR5: 49.339 entradas, aproximadamente 15,41 GiB descompactados. **Não** foi executado teste CRC integral de todas as partes.
- ExeFS recuperado seletivamente da parte 4: `main`, `main.npdm`, `rtld`, `sdk`, `subsdk0`.
- SHA-256 do `main` recuperado: `f6f0b20bb1f2b67d164c24871cd473147eef1d285f12e7bd492e3126c6022e27`. Idêntico ao NSO `main` enviado anteriormente.
- NSO: Build ID `cd57b23fa4bbad65`, reconhecido pelo Savage Wind como BOTW Switch 1.9.0.

## Descobertas da descompilação

O descompilador Ghidra já havia identificado a função em `0x7000B370B0` que consulta uma tabela de **20 IDs climáticos**. A cadeia de funções candidata relacionada à invocação da motocicleta realiza essa verificação antes de concluir a aparição.

A construção da máscara na função utiliza exatamente estas duas instruções:

```asm
0x7000B3711C: mov  w8, #0x9fdf    ; 0x5293fbe8
0x7000B37120: movk w8, #7, lsl #16  ; 0x72a000e8
```

O resultado é `0x79FDF`. Os bits não permitidos (índices 5, 13, 14 e 19) representam, respectivamente, `GerudoDesertClimate`, `EldinClimateLv1`, `EldinClimateLv2` e `GerudoDesertClimateLv2`.

Pseudocódigo reduzido do trecho relevante:

```c
if (other_guard_fails) return false;
unsigned climate_index = lookup_climate(...);
if (climate_index > 19) return true;
return (0x79FDF >> climate_index) & 1;
```

**Limite da inferência:** essa máscara regula a aprovação desses climas, mas ainda falta testar se todas as restrições de regiões do jogo dependem somente dessa verificação.

## Modificação implementada (experimental)

O hook de `tools/patches.py` substitui apenas a segunda instrução **durante a geração do código nativo para PC**. Com `motorcycle_all_regions = 0`, restaura o valor completo `0x79FDF`. Com `1`, atribui `0xFFFFF`, incluindo todos os 20 climas. Não altera nem distribui os arquivos proprietários NSO ou ROMFS.

A implementação só é habilitada na build ID `cd57b23fa4bbad65` e verifica as duas instruções originais. Divergências abortam a geração do código em vez de aplicar um patch indevido. A opção é carregada uma vez na inicialização de `savagewind-cheats.ini`, localizado no diretório de dados do aplicativo. **O padrão é desativado.**

Arquivo de exemplo:

```ini
[cheats]
motorcycle_all_regions = 1
stamps_9999 = 0
```

`stamps_9999` ainda **não está implementado no jogo**: seu ativamento é explicitamente ignorado e avisado no log, para não arriscar a integridade do save. Essa opção exige pesquisa adicional sobre os buffers e serialização.

## Testes realizados nesta sessão

| Teste | Resultado |
|---|---|
| Detecção de 46 partes e catálogo RAR5 | OK (metadados; integridade integral não testada) |
| Extração seletiva dos 5 arquivos ExeFS | OK |
| Hash `main` vs upload anterior | Idênticos |
| Validação da máscara/tabela dos 20 climas | OK |
| Localização da instrução real e Build ID correta | OK |
| Identificação de pontos do jogo por `game_sites.py find` | OK |
| Quatro testes Python de patch/guardas | OK |
| Cinco testes de graphic packs e metadados | OK |
| Teste do carregamento INI em execução | OK |
| Teste AddressSanitizer + UndefinedBehaviorSanitizer (3 cenários) | OK |
| Compilação isolada de `runtime/sw_cheats.c` com `-Werror` | OK |
| Compilação completa do Savage Wind para Windows | NÃO REALIZADA |
| Teste de Master Cycle Zero dentro de BOTW | NÃO REALIZADO |
| Validação integral do RomFS / gameplay | NÃO REALIZADA |

## Obstáculos para validação real

O ambiente de análise não possui o conjunto completo SDL3/Vulkan/FFmpeg para compilar o executável completo; os 15,41 GiB de conteúdo RomFS também excedem o espaço restante para extração integral. Portanto, esta entrega é **código-fonte experimental que passa nos testes estáticos**, não uma build jogável nem um patch já aprovado para instalação sobre o executável Windows.

**Próxima validação**: compilar a biblioteca com o gerador modificado no Windows com seus arquivos do jogo, confirmar a ativação no log e testar regiões permitidas, Gerudo, Eldin, posições não permitidas e o desbloqueio da moto, usando um save de teste (com backup). Medir efeitos nos tempos de quadro e verificar se ainda há bloqueios independentes da máscara de clima.
