# CTF MPS KTX — registro técnico do porte

[English](CHANGES.md) | Português (Brasil)

Estado documentado: **CFN1**, módulo `1.47-ctfnormal.1`, derivado de **KTX 1.47**, executado nos ensaios de integração com **MVDSV 1.11 Windows x64**.

**Criação e manutenção do porte:** Fernando (Droni) Salvatori — [@fernandosalvatori](https://github.com/fernandosalvatori). **Publicação inicial: 28/09/2026.** A atribuição ao porte não substitui os autores do código e dos módulos que lhe deram origem.

O propósito é portar as armas e os efeitos de **CTFNormal / ServerModules** para C nativo KTX: Drone, Shrapnel, WeldGun, Burn, a alteração de Lightning e Hook 1.2. As regras da partida CTF, rede, física e previsão de movimento continuam pertencendo ao KTX/MVDSV. Este registro não afirma equivalência integral entre uma partida NetQuake e uma partida QuakeWorld.

## Origem e atribuição

- KTX: [QW-Group/ktx](https://github.com/QW-Group/ktx), base 1.47, com seus cabeçalhos de autoria e licença preservados.
- Engine dos testes: [QW-Group/mvdsv](https://github.com/QW-Group/mvdsv), versão 1.11, obtida e executada separadamente.
- Módulos portados: ServerModules de **Johannes Plass**, copyright 1996–1997, **GPL versão 2 ou posterior**.
- Referências QuakeC: `_drone.qc`, `_shrap.qc`, `_weldgun.qc`, `_burn.qc`, `_hook.qc`, `_lightng.qc`, respectivos cabeçalhos e pontos de integração em `weapons.qc`, `combat.qc` e `player.qc`.

Os avisos de KTX, QWProgs e código derivado de id Software permanecem por arquivo. [LICENSES/NOTICE.pt-BR.md](../LICENSES/NOTICE.pt-BR.md) identifica a relação entre o código, ferramentas externas e dados do jogo. O porte não substitui a autoria dos componentes por uma atribuição única ao projeto novo.

## Organização e arquivos alterados

| Arquivo | Responsabilidade do porte |
|---|---|
| `source/ktx/include/ctfnormal.h` | Constantes de arma e interfaces públicas da extensão |
| `source/ktx/include/progs.h` | `cfn_state_t`: estado privado, alvos, temporizadores, donos lógicos e cadeia do gancho |
| `source/ktx/include/g_local.h` | Inclusão da interface e identificação da versão personalizada |
| `source/ktx/src/ctfnormal.c` | Ativação, precache, aliases, ajuda, seleção, mensagens, limpeza e wrapper de dano |
| `source/ktx/src/ctfnormal_drone.c` | Fila de drones, navegação, contatos, dano e explosão |
| `source/ktx/src/ctfnormal_shrapnel.c` | Míssil/chama, fragmentos, reflexão, dano e animações |
| `source/ktx/src/ctfnormal_weld.c` | Projéteis incandescentes, luz, impacto e ignição |
| `source/ktx/src/ctfnormal_burn.c` | Camadas de fogo, contágio, chamas, vapor, dor e remoção |
| `source/ktx/src/ctfnormal_hook.c` | Gancho, elos, fixação, tração/balanço e remoção |
| `source/ktx/src/weapons.c` | Integração da seleção, disparos, munição, cadências e Lightning |
| `source/ktx/src/combat.c` | Identificação do golpe letal e tratamento especial de dano a drones |
| `source/ktx/src/player.c` | Dor e animação de morte por fogo, velocidades de gibs e remoção do gancho |
| `source/ktx/src/client.c` | Inicialização/limpeza de cliente, avisos e mensagens de morte |
| `source/ktx/src/commands.c` | Seleção do preset CTF no modo contínuo da extensão |
| `source/ktx/src/world.c` | Registro da cvar, precache e identificação correta do modo CTF |
| `source/ktx/CMakeLists.txt` | Inclusão das unidades C novas |
| `tests/` | Testes determinísticos e fixture específica de integração |
| `scripts/` | Compilação, execução dos testes e preparação local de dados externos |
| `server-example/ktx/` | Configurações de exemplo sem dados do jogo ou credenciais |

O código de teste da engine é condicionado a `CFN_TEST`. Sua entrada serve à validação e não faz parte da interface administrativa da compilação de produção.

## Ativação e ciclo de vida

`CFN_Enabled()` exige `k_ctfnormal` habilitado e `k_mode == 4`. O precache registra explicitamente os modelos e sons usados pelos módulos. Os recursos opcionais de Hook que estavam desativados no fonte de referência permanecem desativados; isso evita depender de arquivos adicionais não previstos naquela configuração.

`ClientConnect` fornece aliases e ajuda. `PutClientInServer` e `ClientDisconnect` chamam a limpeza antes de reutilizar o estado privado. A limpeza retira drones e chamas, solicita remoção do gancho, zera `cfn` e avança a geração do jogador. O gancho verifica sua geração e o vínculo com o jogador para não controlar uma vida ou conexão posterior.

`PlayerPreThink` participa da remoção do gancho quando o jogador morre, teleporta ou sai do modo. Também comunica mudanças no estado de ameaça de Drone por texto.

### Seleção de arma

- A nailgun entra em WeldGun quando selecionada a partir de outra arma; repeti-la alterna Weld/pregos comuns.
- A GL e a RL começam no comportamento convencional; a repetição da seleção alterna Drone ou Shrapnel.
- Mudanças automáticas de arma também passam pela atualização do estado, evitando carregar um modo alternativo incompatível para outra arma.
- Drone e Shrapnel descontam a munição dentro de suas funções de disparo. O fluxo chamador não desconta novamente.
- Weld usa o desconto de um prego do fluxo da nailgun e preserva a correção lateral `ox -= 1` do original.
- A integração mantém as cadências específicas de shotgun, super shotgun, GL e RL no modo CTFNormal. As cadências de disparo são distintas dos intervalos de `think` de projéteis e efeitos.

O sistema usa mensagens `Modo: ...` no lugar do indicador antigo por chave de inventário. Os bits de chave são utilizados pela apresentação das bandeiras no CTF QuakeWorld; reutilizá-los para armas corromperia essa apresentação.

## Drone 1.0

| Elemento | Comportamento/valor portado |
|---|---|
| Custo | 1 foguete |
| Fila | Até quatro drones; um novo lançamento encaminha o mais antigo à explosão |
| Vida | 20 pontos |
| Velocidade inicial | 400 unidades/s |
| Primeiro `think` | 0,6 s após o lançamento |
| Atualizações seguintes | 0,2 s |
| Duração | Lógica original de aproximadamente 6 s, com margem de expiração e resolução do `think` |
| Busca | Direção de voo, visibilidade de posições do alvo, distância e compatibilidade de submersão |
| Movimento | Atualização da posição/velocidade estimada do alvo, ajuste de velocidade e desvios quando a direção fica invertida |
| Colisão | Ricochetes, desgaste por contatos e recuperação de drone parado/no chão |
| Dano radial | Raio 70, base `40 + r × 5`, redução por distância |
| Ignição | `r > 0,95` e dano > 20 |
| Modelo | `progs/lavaball.mdl` |
| Explosão | `progs/s_explod.spr`, quadros 0–5 |

O Drone mantém dono lógico separado do dono físico usado para colisão. A fila e as referências são removidas na limpeza do jogador. O porte também trata drones que estavam perseguindo o jogador removido, para não retargetar involuntariamente um cliente que reutilize o slot.

Foram corrigidos dois problemas de referência: a remoção no céu agora desliga o Drone da fila, e a verificação de busca sem alvo compara a sentinela efetivamente usada (−10). O segundo caso corrige uma comparação histórica com −1. Essas diferenças são deliberadas e não devem ser apresentadas como reprodução literal de um erro do QC.

## Shrapnel 1.0

| Elemento | Comportamento/valor portado |
|---|---|
| Custo | 1 foguete |
| Míssil principal | Velocidade 850, duração máxima 6 s |
| Origem do míssil | Origem do jogador + frente × 36 + Z14 |
| Chama acompanhante | Frente × 18 + Z14, mesma velocidade e duração |
| Impacto no céu | Remove sem explosão |
| Explosão primária | Ao tocar BSP, dano-base `30 + r × 10`, raio de busca dano + 40 |
| Queda primária | Dano − metade da distância ao centro da caixa; metade contra atirador e shambler; exige `CanDamage` |
| Fragmentos | Três normais; quatro conforme o sorteio da explosão primária |
| Velocidade de fragmentos | 600 unidades/s |
| Dano de fragmentos | Raio 70; base `25 + (0,5 − r) × 5` contra `DAMAGE_AIM` |
| Queda dos fragmentos | Base perto do centro, depois `(70 − distância) / 2` até zero |
| Outros objetos danificáveis | 10 pontos |
| Ignição | `r > 0,66` e dano > 6 |
| Temporização de contato BSP | Dano e som com intervalos independentes estritamente maiores que 0,1 s |
| Explosão de fragmento | Quadros 0, 3, 4, remoção; 0,1 s por quadro |

O impacto não BSP não recebe a explosão primária que só existe no caso BSP original. Os fragmentos conservam ricochete e retomada de voo. A fórmula de fragmentos não ganhou uma verificação de linha de visão inexistente no QC e não aplica automaticamente metade do dano contra o próprio atirador.

A autoria do dano usa o dono lógico, mesmo quando o dono físico do fragmento é o mundo. O tempo de conexão do dono é guardado para impedir transferência de crédito após reutilização de slot.

## WeldGun 1.0

| Elemento | Comportamento/valor portado |
|---|---|
| Projétil | `weld_blob`, `MOVETYPE_FLYMISSILE`, caixa de tamanho zero |
| Custo | 1 prego, descontado pelo fluxo de disparo da nailgun |
| Velocidade | 1400 unidades/s |
| Duração máxima | 6 s |
| Origem | Origem do tiro − Z6 + direção × 8 |
| Luz | No máximo um projétil iluminado a cada 0,2 s por atirador |
| Modelo | `progs/flame2.mdl`, ângulo X acrescido de 90 graus |
| Raio | 60 |
| Dano contra `DAMAGE_AIM` | `db = 11 + (0,5 − r) × 6`, entre 8 e 14 |
| Distância de dano | Origem do alvo + Z16 até o impacto |
| Queda | Dano cheio abaixo de `3 × db`, queda linear até zero em 60 |
| Outros objetos danificáveis | 10 pontos |
| Ignição | A mesma amostra `r` do dano: estritamente > 0,85, com dano > 5 |
| Explosão | Recua 4 unidades; quadros 0, 3, 4 em `s_explod.spr`, 0,1 s cada |

Impactos no céu são removidos sem dano. Mantidos sons originais de disparo/voo/impacto. A comparação `0.85f` preserva a precisão float da constante QuakeC: não deve virar uma comparação contra uma constante double que inclua o valor de borda por arredondamento.

Não se acrescentou verificação de parede no dano radial, pois a rotina original também não a possuía. O dano final passa pelos modificadores normais do combate KTX.

## Burn 1.0

### Ignição e camadas

A ignição é bloqueada quando a vítima está submersa acima da cintura, invulnerável, morta, é um Drone ou um barril explosivo. Em CTF, não incendeia outro integrante da mesma equipe; autoignição continua permitida conforme as regras originais.

As camadas usam bits 1, 2 e 4, cada uma com duração de 15 segundos. Cada camada contribui com 3 pontos por tick de dano; três camadas somam 9 antes de armadura e outros modificadores. O primeiro `think` ocorre em 0,1 s; dano usa o temporizador de 1 s e a comparação estrita `time > burn_damage_time`. A parte visual atualiza em 0,02 s.

Uma camada expirada ainda contribui no tick em que seu bit é removido, conforme o QC. Quando as três camadas estão ocupadas, foram preservadas as comparações aninhadas originais de renovação, incluindo o caso em que nenhuma camada é renovada. Não se apresenta essa peculiaridade como escolha nova de balanceamento.

### Contágio e água

Contágio usa centro na origem da vítima + Z18, raio 50, dano `6 + r × 4` e ignição quando `r > 0,5`. O mesmo sorteio é usado para os vizinhos desse tick. O crédito do contágio pertence ao personagem em chamas; o dano direto das camadas continua atribuído ao autor inicial.

Água com `waterlevel > 1` apaga no próximo tick de dano, produz som de extinção e oito bolhas em intervalos de 0,1–0,3 s. As bolhas começam com velocidade Z15 e passam à rotina nativa `bubble_bob`.

### Chamas, dor e morte

São duas chamas em `flame2.mdl`, quadro 1, próximas ao personagem e atrás da direção de visão, com variação oposta de posição. A chama principal produz luz. A altura é reduzida durante a animação de morte.

`PainSound` usa os sons de queimadura com intervalo mínimo de 0,8 s. Uma morte por outra arma remove as chamas; morte por Burn mantém o efeito até `DEAD_DEAD` e termina com uma animação de explosão 0–5.

O fluxo do jogador utiliza `player_dieb1`, depois gibs. A velocidade dos gibs parte da velocidade atual, acrescentando ±80 em X/Y e 50–100 em Z. `ThrowHead` preserva altura e velocidade particulares da morte por fogo. A marca `burn_gibbed` impede gibs duplicados no fluxo KTX quando já foram produzidos antes de `PlayerDead`.

As chamas só são limpas na mudança de vida/desconexão ou ao terminar o efeito, não no começo de `PlayerDie`. O descarte da bandeira permanece no fluxo nativo CTF.

## Lightning 1.1 e Hook 1.2

### Lightning

A descarga especial exige `waterlevel > 2`. Sua intensidade-base é `min(400, 20 × células)` e ela consome as células restantes. A distância, armadura e demais regras continuam sendo aplicadas; 400 não deve ser descrito como dano final garantido para todos os alvos.

Os sons são controlados por temporizadores e deslocamento do ponto atingido. O teste de movimento considera intervalo mínimo de 0,1 s, mudança acima de 10 unidades e sorteio > 0,3; a repetição regular usa a janela de 0,6 s do original.

### Hook

Impulses 98/97 implementam segurar/liberar. O impulse 22 é um atalho adicional para alternar o mesmo sistema. O gancho viaja a 1400 unidades/s, gera oito elos e causa 7 pontos de dano no contato antes dos modificadores. A fixação num personagem dura no máximo 2 s.

Tração e balanço são calculados pelo `think` original em intervalos de 0,1 s, compondo as velocidades paralela e tangencial em relação ao ponto de fixação. O efeito não é substituído pelo gancho nativo KTX. Morte, teleporte e liberação terminam o vínculo.

O código de referência tinha comentado o bloco que ativava `HOOK_FLY`; o porte não o reativa como comportamento novo. Modelos e sons opcionais customizados que estavam desativados na referência continuam desativados. Permanecem os recursos padrão `v_spike.mdl`, `s_spike.mdl` e sons correspondentes.

## Adaptações de API e correções de segurança de referência

1. **Estado QC para C:** campos particulares vivem em `gedict_t.cfn`; `owner`/`enemy` nativos usam `EDICT_TO_PROG` e `PROG_TO_EDICT`. Funções de estado QuakeC `[frame, next]` viraram callbacks com `nextthink` explícito.
2. **Busca por raio:** a cadeia retornada pelo builtin NetQuake foi substituída pela iteração incremental de `trap_findradius` do KTX. Centros, raios e filtros específicos foram mantidos.
3. **Dano comum:** `CFN_Damage` classifica a arma e encaminha a `T_Damage`; não contorna armadura, Quad e proteção de equipe dos jogadores. O golpe letal registra `cfn.killweapon` antes de `Killed`.
4. **Slots reutilizados:** Weld, Shrapnel e Burn registram a conexão do atacante. Se outra pessoa ocupa o mesmo slot, o dano pendente passa a ser atribuído ao mundo. Respawn legítimo mantém o crédito, pois não muda a conexão.
5. **Limpeza de efeito:** referências de chamas são anuladas ao terminar. A segunda chama e o emissor de vapor têm identificação própria. Vapor não acompanha um cliente novo que reutilize o slot da vítima.
6. **Gancho:** vínculo e geração evitam um gancho antigo modificar o estado de uma vida posterior. Corrente e callbacks são removidos junto com o gancho.
7. **Drone:** limpeza de fila no céu e correção da sentinela sem alvo impedem uso de referências incorretas.
8. **Morte:** identificação da arma somente no golpe letal e proteção contra duplicação de gibs preservam o estado da morte mesmo diante de chamadas adicionais sem dano efetivo.

Esses ajustes devem ser distinguidos das fórmulas históricas mantidas. Uma intenção de fidelidade não exige preservar referências inválidas ou atribuir dano ao ocupante errado de um slot.

## Configuração e carregamento CTF

O exemplo usa CTF contínuo com `k_matchless 1`. Quando `k_ctfnormal` está ativo, a seleção automática do modo escolhe o preset CTF. A publicação de `mode=ctf` considera também o estado nativo `isCTF()` para evitar um rótulo de preset anterior no primeiro carregamento.

A inicialização comum de presets limpa `sv_loadentfiles_dir`. As regras do modo restauram `sv_loadentfiles 1` e `sv_loadentfiles_dir ctf` diretamente nos arquivos apropriados. Apenas definir essas opções antes de um preset não era suficiente para garantir o carregamento das entidades.

A configuração de referência fornece limite 150, tempo 40, velocidade 350 e aceleração 20. São opções do exemplo e não requisitos do código. `teamplay 4` representa proteção de vida/armadura de aliados com dano próprio; a antiga máscara ServerModules não é reutilizada como número de teamplay KTX.

Bots e runas ficam desativados, assim como o gancho nativo KTX. A compilação pode conter estruturas do suporte nativo de bots necessárias à base KTX; isso não cria bots nem popula o servidor. Os clientes sintéticos da fixture são exclusivos de teste.

Os dados de mapa são externos. A preparação de uma instalação usa arquivos fornecidos localmente e não altera o original. Correções de consistência de entidades devem ser verificadas contra o BSP correspondente: um nome de submodelo não identifica a mesma geometria em mapas diferentes.

O importador aplica dois ajustes nas cópias locais de entidades: em `e4m4`, o destino de teleporte `t204` usa origem `1065 758 273` e ângulos `30 102 0`, para que o acréscimo nativo de Z27 durante o spawn resulte em `1065 758 300`; em `e4m2`, remove chaves comuns ativas no deathmatch, sem remover as entidades de bandeira. Isso reproduz ajustes que o código CTFNormal fazia após o carregamento. O importador não distribui esses arquivos nem converte esses dados em conteúdo do repositório.

## Validação registrada

Os números abaixo descrevem o estado CFN1. As três suites unitárias foram executadas novamente na árvore pública: **3674 verificações passaram** em Windows x64 com Zig 0.13.0. A compilação nativa e o carregamento da DLL, com exports `vmMain` e `dllEntry`, também passaram. As 27 verificações na engine e a varredura de 20 mapas são registros anteriores do mesmo estado, não parte do comando unitário. As suites devem ser executadas novamente ao mudar código, compilador ou integrações relevantes.

| Conjunto | Total | Escopo |
|---|---:|---|
| WeldGun/Burn | 3324 | Módulos C reais; mock da fronteira KTX/engine |
| Shrapnel | 252 | Módulo C real; entradas e respostas determinísticas |
| Drone/Hook | 98 | Módulos C reais; fila, alvos, movimento e estados |
| Total unitário | **3674** | Não substitui física/renderização da engine |
| Integração MVDSV/KTX | **27** | Fixture carregada na engine, zero falhas |
| Rotação | **20 mapas** | Carregamento, status, modo CTF e duas bandeiras |

### O que os testes unitários exercitam

- **Weld/Burn:** 280 combinações de aleatoriedade/distância de impacto, fórmula radial, limiares estritos, voo, luz, vida, quadros, bloqueios de ignição, camadas/renovação/expiração, autoria do contágio, água/bolhas, morte, limpeza idempotente, sons e identidade de conexão.
- **Shrapnel:** lançamento/custo, modelo/propriedade, céu, BSP e não BSP, dispersão e número de fragmentos, fórmulas de dano, ignição, exceção shambler, obstáculos da explosão primária, sangue, limites de frequência, ricochete, retomada do voo, expiração, animações e reutilização do slot do dono.
- **Drone/Hook:** caminhos portados com respostas determinísticas da engine, incluindo limite da fila, seleção/atualização do alvo, estados de contato, gancho/corrente e remoção.

### O que a fixture na engine exercita

1. CTF nativo ativo e as duas bandeiras presentes.
2. Seleção Weld ao entrar na nailgun e retorno para pregos comuns.
3. Criação do projétil, munição e velocidade Weld.
4. Alternância da GL para Drone, custo e limite de quatro drones.
5. Limpeza da fila de drones.
6. Alternância da RL para Shrapnel, custo e velocidade.
7. Ignição, perda real de vida por `T_Damage` e extinção na água.
8. Morte por Burn, gibs, reset no respawn e morte convencional posterior.
9. Proteção de aliados contra dano/ignição e preservação de dano próprio.
10. Consumo das células e limite da descarga submersa da Lightning.
11. Lançamento/liberação do gancho por impulses 98/97.

Esses grupos contêm 27 verificações individuais. A fixture é compilada com `CFN_TEST`; sua entrada não deve ser incluída na compilação normal.

### Varredura de mapas

Foram carregados `e1m1`, `e1m2`, `e1m3`, `e1m4`, `e1m5`, `e1m6`, `e2m1`, `e2m2`, `e2m3`, `e2m5`, `e3m1`, `e4m3`, `e4m4`, `e4m5`, `e4m6`, `dm1`, `dm3`, `dm4`, `dm5` e `dm6`.

O ensaio confirmou resposta de status com o mapa correto, CTF ativo e ambas as bandeiras. Não percorreu visualmente cada rota e não realizou uma partida completa em cada mapa. Os BSP/ENT usados nesse ensaio não são distribuídos no repositório.

### Limites da validação

Ainda não estão demonstrados:

- uma partida humana completa com verificação visual e de controle de todas as armas;
- igualdade de colisões, ricochetes, mira e sensação de movimento com o NetQuake original;
- captura/retorno de bandeira em todas as combinações de efeitos, gancho e mortes;
- matriz completa de mudanças de equipe, inventário/HUD, latência e vários clientes;
- carga prolongada de partidas e equivalência em outros sistemas operacionais/arquiteturas.

O projeto registra o que foi efetivamente testado. Contagem de assertions, compilação bem-sucedida e carregamento de mapas não são usados como substitutos de um playtest humano.

## Regressões e revisão futura

Pull requests deste repositório exigem revisão e aprovação final de [@fernandosalvatori](https://github.com/fernandosalvatori). Outras revisões ajudam a avaliar as alterações, mas não substituem a aprovação exclusiva do mantenedor para incorporá-las. O formato esperado de contribuição está em [CONTRIBUTING.pt-BR.md](../CONTRIBUTING.pt-BR.md).

Mudanças neste porte devem preservar as suites existentes e acrescentar casos dirigidos quando alterarem dano, seleção, ciclo de vida ou propriedade de entidades. Os pontos de maior risco são estados que sobrevivem ao respawn/desconexão, callbacks de dano que disparam morte, fila/corrente de entidades e carregamento de regras na troca de mapa.

Uma proposta de integração upstream deve separar código de arma das preferências de configuração, manter autoria/licença dos ServerModules, apresentar a diferença contra a base KTX e incluir resultados de jogo humano quando disponíveis. Dados comerciais do jogo, recursos externos e configurações privadas não pertencem ao patch de código.

A árvore pública documentada aqui não indica que um PR foi aceito ou que os mantenedores originais aprovaram esta adaptação.
