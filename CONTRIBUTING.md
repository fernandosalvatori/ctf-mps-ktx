# Contribuir com CTF MPS KTX

O criador e mantenedor deste porte é **Fernando (Droni) Salvatori**, [@fernandosalvatori](https://github.com/fernandosalvatori). O projeto foi publicado inicialmente em **28/09/2026**, preservando a autoria preexistente de KTX, MVDSV, ServerModules e demais componentes.

## Revisão e aprovação

**Todo pull request de outros contribuidores exige revisão e aprovação de @fernandosalvatori antes de ser incorporado a este repositório. A aprovação final necessária é exclusiva do mantenedor.** Revisões de outras pessoas e testes automatizados podem apoiar a análise, mas não substituem essa aprovação.

O GitHub não permite aprovar o próprio PR. PRs criados pelo mantenedor podem ser incorporados por ele mediante bypass administrativo, sempre por PR. Esse bypass também permite ao proprietário integrar outros PRs por decisão própria; não registra uma aprovação de revisão fictícia. Não há outros colaboradores com permissão de escrita na publicação inicial.

A abertura de um PR, a passagem dos testes ou uma discussão favorável não significam aprovação. Após mudanças relevantes no diff, apresente novamente os resultados e aguarde a revisão da versão final. O mantenedor decide se a alteração entra no projeto e em qual versão.

Essa política governa a incorporação de contribuições neste repositório. Ela não altera a licença do código nem declara aprovação por parte dos projetos upstream.

## Preparar uma alteração

Mantenha o PR concentrado em um problema ou mudança de comportamento identificável. Preserve os cabeçalhos de licença e autoria. Antes de mudar uma fórmula ou regra de arma, consulte [docs/CHANGES.md](docs/CHANGES.md) e identifique se o comportamento vem de KTX, de ServerModules ou de uma adaptação deliberada deste porte.

Quando a mudança afetar várias camadas, explique a ligação entre elas: por exemplo, seleção da arma, gasto de munição, criação de projétil, dano, morte e limpeza no respawn. Evite misturar preferências de configuração do servidor com correções de código que possam ser avaliadas separadamente.

## O que incluir na descrição do PR

- **Problema ou objetivo:** situação concreta que motivou a alteração e resultado esperado.
- **Antes e depois:** comportamento observável, com um exemplo que o revisor possa reproduzir.
- **Diff explicado:** arquivos e funções relevantes, o motivo das alterações e possíveis efeitos sobre outros modos.
- **Origem das regras:** parâmetros ou rotinas de referência, com autoria e indicação clara das diferenças deliberadas.
- **Validação:** comandos executados, versões de compilador/engine, resultados e casos de regressão cobertos.
- **Limites:** cenários não testados, diferenças conhecidas e o que ainda depende de partida humana ou verificação visual.

Uma descrição clara deve permitir a revisão sem depender de conversas privadas. Não apresente compilação, contagem de assertions ou carregamento de mapas como comprovação de uma partida completa.

## Compilação e testes

O fluxo documentado usa Python 3.9 ou posterior e Zig 0.13.0 em Windows x64. Na raiz do repositório:

```text
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

Os scripts aceitam um caminho de executável em `--zig`; consulte [docs/BUILD.md](docs/BUILD.md) para `ZIG_EXE`, saídas e opções. Registre o resultado da execução atual, em vez de copiar uma contagem histórica sem rodar os testes.

Para alterações de dano, seleção ou ciclo de vida, acrescente casos que reproduzam o problema e exerçam os efeitos da correção. Dê atenção a respawn, desconexão, reutilização de slot, dano aliado/próprio, callbacks de morte e remoção de entidades. Mudanças de projéteis ou gancho também precisam de validação na engine quando afetarem colisão ou movimento.

`tests/runtime.c` é uma fixture separada de integração, não parte da DLL normal produzida pelo comando de compilação acima. Se usar instrumentação, identifique-a nos resultados e mantenha-a fora do módulo de produção.

Não afirme teste visual, partida humana ou execução remota de CI que não tenha ocorrido. Se uma etapa não puder ser feita, descreva a limitação de forma explícita.

## Conteúdo que não deve entrar no PR

- PAKs, mapas BSP/ENT, modelos, sons ou outros assets de instalações locais e do jogo comercial.
- Executáveis da engine, compilações, caches ou artefatos gerados que não façam parte do código revisável.
- Credenciais, configurações privadas, tokens, endereços de serviços pessoais ou dados de jogadores.
- Cópias de instalações pessoais, logs completos ou arquivos de diagnóstico sem revisão do conteúdo.

Os recursos externos necessários aos testes de engine devem ser fornecidos localmente. Compartilhe no PR apenas o código, os passos e a evidência necessária à revisão, removendo dados privados dos trechos de log. O importador local não concede licença para redistribuir os arquivos importados.

## Autoria e licença

Conserve as atribuições de **Johannes Plass** nos módulos ServerModules e as dos demais autores nos componentes KTX e código anterior. Identifique a autoria e a origem de código novo ou adaptado sem atribuir alterações deste porte aos autores upstream.

Leia [LICENSE.md](LICENSE.md) e [LICENSES/NOTICE.md](LICENSES/NOTICE.md). Se a contribuição depender de código ou conteúdo de outra origem, informe essa origem e seus termos para que a compatibilidade possa ser avaliada durante a revisão.
