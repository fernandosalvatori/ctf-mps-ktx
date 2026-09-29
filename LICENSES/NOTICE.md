# Origem, autoria e licenças

Este documento identifica as principais origens do código de **CTF MPS KTX**. Ele complementa os cabeçalhos existentes; não substitui nem remove seus termos.

## KTX e código anterior

A árvore `source/ktx` deriva de **KTX 1.47**, projeto [QW-Group/ktx](https://github.com/QW-Group/ktx). O texto da GNU General Public License versão 2 está em [../LICENSE.md](../LICENSE.md), copiado da distribuição KTX.

Os arquivos KTX preservam avisos de seus autores e contribuições anteriores. Entre eles há cabeçalhos QWProgs-DM com copyright de `[sd] angel` e referências a código QuakeWorld/Quake de **id Software, Inc.**, além de outras atribuições por arquivo. Esses avisos não foram substituídos por uma atribuição única ao porte.

Consulte também a documentação e os avisos individuais da árvore upstream preservada. Alguns componentes auxiliares podem conter condições ou atribuições próprias; o resumo deste documento não redefine a licença desses componentes.

## ServerModules

As regras especiais foram portadas de módulos QuakeC do **CTFNormal / ServerModules**, de **Johannes Plass**, copyright **1996, 1997**, cujos cabeçalhos permitem redistribuição e modificação sob a **GNU GPL versão 2 ou, a critério do destinatário, qualquer versão posterior** (`GPL-2.0-or-later`).

| Origem QuakeC | Porte em C |
|---|---|
| `_drone.qc`, `_drone.qh` — Drone 1.0 | `source/ktx/src/ctfnormal_drone.c` |
| `_shrap.qc`, `_shrap.qh` — Shrapnel 1.0 | `source/ktx/src/ctfnormal_shrapnel.c` |
| `_weldgun.qc` — WeldGun 1.0 | `source/ktx/src/ctfnormal_weld.c` |
| `_burn.qc` — Burn 1.0 | `source/ktx/src/ctfnormal_burn.c` |
| `_hook.qc`, `_hook.qh` — Hook 1.2 | `source/ktx/src/ctfnormal_hook.c` |
| `_lightng.qc`, `_lightng.qh` — Lightning 1.1 | Integração em `ctfnormal.c` e `weapons.c` |
| Integrações de `weapons.qc`, `combat.qc`, `player.qc` | Seleção, dano, contatos, dor e morte nos arquivos KTX correspondentes |

Os novos arquivos de módulos preservam a autoria e a indicação GPL. A conversão para C nativo, as interfaces KTX, a limpeza de referências, os testes e a documentação são alterações deste porte e não devem ser atribuídos como comportamento originalmente escrito pelos autores upstream.

## Engine e ferramentas

**MVDSV 1.11** é uma dependência externa, obtida separadamente de [QW-Group/mvdsv](https://github.com/QW-Group/mvdsv). Seu executável não é distribuído neste repositório. A licença e os avisos da distribuição MVDSV continuam aplicáveis à engine.

Python e Zig são ferramentas externas de compilação/teste; não fazem parte da distribuição de código deste projeto e conservam suas próprias licenças.

## Dados externos não distribuídos

O repositório não inclui:

- PAKs de Quake;
- mapas BSP e arquivos de entidades das instalações locais;
- modelos e sons do jogo ou pacotes de recursos de terceiros;
- executáveis da engine;
- configurações privadas, credenciais ou cópias de instalações pessoais.

O script de preparação usa arquivos locais fornecidos pelo operador. A presença de um importador não concede permissão adicional para usar ou redistribuir esses arquivos. A GPL do código do mod não converte automaticamente os dados comerciais do jogo em conteúdo GPL.

## Relação com os projetos originais

**CTF MPS KTX** identifica esta adaptação independente. Não implica endosso de KTX, MVDSV, id Software ou dos autores de ServerModules. Os nomes dos projetos são usados para identificar a origem e a compatibilidade pretendida.
