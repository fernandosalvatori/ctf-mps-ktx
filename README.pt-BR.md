# CTF MPS KTX

[English](README.md) | Português (Brasil)

Porte das armas especiais do **CTFNormal / ServerModules** para **QuakeWorld**, usando **KTX 1.47** e **MVDSV 1.11**. O módulo é identificado como `1.47-ctfnormal.1` e acrescenta **Drone, Shrapnel e WeldGun**, com **Burn, as alterações de Lightning e o gancho original**.

**Criador e mantenedor deste porte:** Fernando (Droni) Salvatori — [@fernandosalvatori](https://github.com/fernandosalvatori). **Publicação inicial do CTF MPS KTX: 28/09/2026.** Essa atribuição identifica a criação e manutenção do porte; a autoria preexistente de KTX, MVDSV, ServerModules e dos demais componentes permanece preservada.

O CTF, a pontuação, o gerenciamento das equipes e o protocolo permanecem os do KTX/MVDSV. A configuração de exemplo não popula a partida com bots e mantém as runas desativadas. Este repositório reúne código, testes, documentação e configurações de exemplo; **não distribui PAKs, mapas, modelos, sons nem o executável da engine**.

É uma adaptação em desenvolvimento: há testes automatizados dos módulos e testes na engine, mas **não foi demonstrada equivalência integral de uma partida ao NetQuake original nem concluído um teste visual com jogador humano**.

## Recursos e controles

Os números são os impulses padrão de seleção. As teclas correspondentes dependem dos binds do cliente.

| Recurso | Comando | Comportamento |
|---|---|---|
| WeldGun | `impulse 4` | Ao selecionar a nailgun, entra em WeldGun; repita para alternar com pregos comuns. Consome 1 prego por disparo. |
| Drone | `impulse 6` novamente com a GL selecionada | Alterna granada/Drone. Consome 1 foguete; mantém até quatro drones por jogador. |
| Shrapnel | `impulse 7` novamente com a RL selecionada | Alterna foguete/Shrapnel. Consome 1 foguete e libera fragmentos incendiários. |
| Lightning | `impulse 8` | Arma elétrica com sons e descarga submersa do módulo original. |
| Gancho | `+hook` / `-hook` | Segurar lança e mantém a tração; soltar libera. Impulses originais 98/97. |
| Gancho alternado | `impulse 22` | Atalho adicional para lançar/liberar o mesmo gancho. |
| Burn | Automático | Incêndio com até três camadas, contágio, extinção na água e efeitos de morte. |

Exemplo opcional para o console do cliente:

```text
alias +hook "impulse 98"
alias -hook "impulse 97"
bind mouse3 +hook
```

O servidor fornece os aliases, mas não substitui os binds do teclado. Também fornece `help-drone`, `help-shrapnel`, `help-weldgun` e `help-hook`, correspondentes aos impulses 215, 216, 217 e 219.

### Comportamento das armas

- **Drone:** projétil teleguiado, com vida própria, seleção e atualização de alvo, adaptação de velocidade, ricochete e explosão. Um quinto lançamento encaminha o drone mais antigo à explosão. O algoritmo foi portado do ServerModules, não de uma inteligência de bots.
- **Shrapnel:** míssil com chama acompanhante e três ou quatro fragmentos conforme o impacto/sorteio original. Os fragmentos ricocheteiam, causam dano em área e podem incendiar alvos.
- **WeldGun:** projéteis de metal incandescente, rápidos, com dano em área e chance de iniciar Burn.
- **Burn:** até três camadas de fogo, cada uma com temporizador próprio. Causa dano periódico e pode espalhar fogo para personagens próximos. A água acima da cintura extingue no próximo tick de dano.
- **Lightning:** a descarga aquática exige submersão completa e usa intensidade-base limitada a 400, consumindo as células. Os sons reagem ao deslocamento do ponto atingido e aos temporizadores originais.
- **Gancho:** corrente com oito elos, fixação em superfícies/entidades, dano de contato, tração e balanço do ServerModules Hook 1.2. O gancho nativo KTX permanece desativado para não haver dois sistemas sobrepostos.

## Estrutura do repositório

```text
source/ktx/          KTX 1.47 com o porte em C
tests/              Testes dos módulos e fixture de integração
scripts/            Compilação, testes e preparação de uma instalação local
server-example/ktx/ Configurações de exemplo, sem credenciais
docs/CHANGES.md      Detalhamento do porte, adaptações e validação
LICENSE.md          Texto da GNU GPL versão 2
LICENSES/NOTICE.md  Origem e avisos de autoria/licença
```

O código original dos módulos foi convertido de QuakeC para C nativo. Ele não executa um segundo `progs.dat` dentro do KTX. O executável MVDSV é uma dependência separada.

## Compilar e testar

O caminho de compilação usado neste porte é **Windows x64**, **Python 3.9 ou posterior** e **Zig 0.13.0**. A existência de outros alvos no KTX upstream não significa que este conjunto de modificações foi validado neles.

Disponibilize `zig` no `PATH` ou substitua o argumento `--zig` pelo caminho do executável. Na raiz do repositório:

```text
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

A compilação gera `build/qwprogs.dll`; os testes gravam o resumo em `build/tests/results.json`. Ambos aceitam `ZIG_EXE` e `--out-dir`; detalhes em [docs/BUILD.md](docs/BUILD.pt-BR.md).

Os testes unitários usam os arquivos C reais e substituem a fronteira da engine por respostas determinísticas. A fixture de integração é separada e só deve entrar numa compilação de teste; não é um comando administrativo da compilação normal.

## Preparar uma instalação local

Você precisa fornecer arquivos de uma instalação de Quake e do CTFNormal que possa usar, um executável MVDSV 1.11 obtido separadamente e os recursos de KTX 1.47. O script de preparação **importa arquivos locais**; não baixa dados do jogo nem a engine.

O diretório informado por `--quake-dir` deve conter `id1/pak0.pak` e `id1/pak1.pak`. `--ctfnormal-dir` aponta para o mod de referência com sua subpasta `Maps`. `--ktx-assets` aponta para `resources/example-configs/ktx` de uma distribuição KTX 1.47 que contenha os modelos e sons necessários.

Exemplo, usando caminhos relativos que devem ser substituídos pelos seus:

```text
python scripts/prepare_server.py --quake-dir ../quake --ctfnormal-dir ../ctfnormal --mvdsv ../mvdsv/mvdsv.exe --ktx-assets ../ktx-1.47/resources/example-configs/ktx --output ../ctf-mps-runtime
```

Use um **diretório de saída novo**. Mantenha a instalação de origem separada para comparação. A preparação utiliza `build/qwprogs.dll` por padrão; `--progs` permite informar outro módulo compilado. Consulte `python scripts/prepare_server.py --help` para as demais validações da preparação.

Depois de preparar a instalação, use `Iniciar.cmd`, `Status.cmd` e `Parar.cmd`. Eles operam apenas a instância registrada pelo próprio launcher, verificando o executável e o horário de início do processo antes de encerrá-lo; não dependem de RCON. Também é possível iniciar o MVDSV diretamente no diretório de execução com o módulo KTX compilado:

```text
mvdsv.exe -basedir . -game ktx -port 27561 +exec server.cfg +map e1m1
```

Use um cliente **QuakeWorld**. Para conectar ao servidor no mesmo computador:

```text
disconnect
spectator 0
team blue
connect localhost:27561
```

Troque `blue` por `red` para escolher a outra equipe. Os comandos `tblue` e `tred` do mod NetQuake não são os comandos de entrada do KTX. A configuração de exemplo não cria serviço, tarefa de inicialização nem regra de encaminhamento no roteador.

## Configuração

| Opção | Exemplo | Finalidade |
|---|---|---|
| `k_ctfnormal` | `1` | Habilitar a extensão |
| `k_mode` | `4` | CTF |
| `k_matchless` | `1` | Partida contínua |
| `teamplay` | `4` | Proteger vida/armadura de aliados e preservar dano próprio |
| `fraglimit` / `timelimit` | `150` / `40` | Limites da configuração de referência; podem ser alterados |
| `sv_maxspeed` / `sv_accelerate` | `350` / `20` | Valores da configuração de referência |
| `k_fb_enabled` | `0` | Desativar bots |
| `k_ctf_runes` | `0` | Desativar runas |
| `k_ctf_hook` | `0` | Desativar o gancho nativo KTX; usar o gancho portado |
| Porta de execução | UDP `27561` | Instância de exemplo |

Os valores 150/40, hostname, porta e rotação são preferências de configuração, não requisitos das armas. A máscara de módulos `teamplay` do antigo ServerModules não pode ser copiada diretamente para uma regra de equipe do KTX.

As configurações do modo devem restaurar `sv_loadentfiles 1` e `sv_loadentfiles_dir ctf`, pois a inicialização de presets KTX pode limpar esse diretório. Os arquivos de exemplo mantêm os valores do CTF coerentes com a extensão. Configurações privadas de administração pertencem à instalação local e não ao repositório.

## Diferenças e limites conhecidos

**Física e protocolo:** movimento, previsão do cliente, mira e simulação são QuakeWorld/MVDSV. Preservar constantes das armas não torna a experiência idêntica à de NetQuake. A cadência antiga da engine não foi copiada literalmente.

**HUD:** ServerModules usava bits de chaves para indicar arma alternativa e ameaça de Drone. Esses bits conflitam com a apresentação de bandeiras no CTF QuakeWorld. O porte guarda os estados separadamente e usa mensagens de modo/alerta; não promete o mesmo ícone do cliente antigo.

**Escopo:** administração, ranking, menus e randomização de itens do pacote ServerModules não foram integralmente portados. O CTF nativo KTX continua responsável pela partida. O módulo opcional de proteção de spawn estava desativado na configuração de referência e não foi acrescentado.

**Dados:** o código e a configuração não substituem os recursos de mapa/modelo/som necessários. Não há autorização implícita para redistribuir dados comerciais de Quake junto com este projeto.

## Validação registrada

| Conjunto | Resultado |
|---|---|
| WeldGun/Burn | 3324 verificações dos módulos C reais |
| Shrapnel | 252 verificações dos módulos C reais |
| Drone/Hook | 98 verificações dos módulos C reais |
| Total unitário | **3674 verificações** |
| MVDSV/KTX integrado | **27 verificações**, zero falhas |
| Mapas | **20 mapas** carregados, com status correto, CTF ativo e duas bandeiras |

As **3674 verificações unitárias foram executadas novamente nesta árvore pública**, em Windows x64 com Zig 0.13.0, sem falhas. A DLL também foi compilada e carregada, com os exports `vmMain` e `dllEntry` verificados. Os resultados de **27 verificações na engine e 20 mapas** são registros anteriores do mesmo estado CFN1; não são executados pelo comando unitário acima. A integração exercitou seleção, munição, projéteis, fila de drones, dano real, Burn/água/morte/respawn, proteção de aliados, dano próprio, Lightning submersa e impulses do gancho. A varredura de mapas usa dados externos não distribuídos aqui.

**Ainda falta teste humano visual completo**, incluindo partidas, captura de bandeira, sensação do gancho, colisões, áudio e comportamento em rede. Os 20 mapas carregados não equivalem a 20 partidas testadas.

Veja [docs/CHANGES.md](docs/CHANGES.pt-BR.md) para fórmulas, pontos de integração, diferenças deliberadas e limites dos testes.

## Origem e licença

Baseado em [KTX](https://github.com/QW-Group/ktx) e destinado a [MVDSV](https://github.com/QW-Group/mvdsv). Os módulos ServerModules portados são de **Johannes Plass, 1996–1997**, licenciados sob GPL versão 2 ou posterior. Os avisos do KTX, QWProgs e código derivado de id Software permanecem nos respectivos arquivos.

Consulte [LICENSE.md](LICENSE.md) e [LICENSES/NOTICE.md](LICENSES/NOTICE.pt-BR.md). A licença do código não deve ser confundida com a licença dos dados externos do jogo. Este projeto não afirma aprovação ou integração nos projetos oficiais.

## Contribuições e aprovação de PRs

Pull requests exigem revisão e aprovação de **Fernando (Droni) Salvatori, [@fernandosalvatori](https://github.com/fernandosalvatori)**, antes da incorporação neste repositório. A aprovação final necessária é exclusiva do mantenedor; revisões de outras pessoas não a substituem.

Consulte [CONTRIBUTING.md](CONTRIBUTING.pt-BR.md) para apresentar alterações, testes e limitações de forma revisável.
