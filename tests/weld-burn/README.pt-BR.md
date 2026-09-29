[English](README.md) | Português (Brasil)

# WeldGun e Burn: porte CTFNormal para KTX

Origem: `_weldgun.qc` e `_burn.qc` do CTFNormal / ServerModules, módulos 1.0 de Johannes Plass, copyright 1996–1997, GPL versão 2 ou posterior. Os arquivos C portados preservam a autoria e a licença; ver [NOTICE](../../LICENSES/NOTICE.pt-BR.md).

## Mecânica preservada

### WeldGun

- Projétil `weld_blob`, `MOVETYPE_FLYMISSILE`, `SOLID_BBOX`, tamanho zero, velocidade 1400 e vida máxima 6 segundos.
- Origem = origem do tiro − `(0,0,6)` + direção × 8; ângulo do modelo em X acrescido de 90 graus.
- Modelo `progs/flame2.mdl`; iluminação no máximo uma vez por 0,2 segundo por atirador.
- Sons originais `weapons/spike2.wav` (volume 0,6), `hknight/idle.wav` e impacto `wizard/hit.wav`.
- Impactos no céu são removidos sem explosão/dano. Outros impactos geram sangue de intensidade 9 quando a entidade tocada recebe dano.
- Raio de dano 60. Alvos sem `DAMAGE_AIM` recebem 10. Outros recebem `db = 11 + (0,5 − r) × 6`, entre 8 e 14. Distância medida do impacto até origem do alvo + `(0,0,16)`: dano cheio abaixo de `3 × db`, queda linear até zero em 60.
- Ignição somente se a MESMA amostra aleatória do dano for estritamente maior que 0,85 e o dano calculado for estritamente maior que 5. A constante é `0.85f` para preservar a comparação float do QuakeC.
- Não se introduziu verificação de visibilidade no dano de área: o QC original também não tem.
- Impacto recua 4 unidades no vetor de movimento, imobiliza o projétil e mostra `progs/s_explod.spr`, quadros 0, 3 e 4 por 0,1 segundo cada; depois remove.
- Seleção, custo de munição, cadência e animação do jogador pertencem à integração em `weapons.c`, não a `CFN_WeldFire` (igual à separação original).

### Burn

- Bloqueia ignição se submersão > 1, invulnerabilidade vigente, drone, barril explosivo, alvo morto ou aliado diferente do próprio atacante.
- Ignição própria permanece permitida. Usa times nativos KTX em lugar do campo `ctf_team` do QC.
- Até três camadas independentes, bits 1/2/4. Cada nova camada dura 15 segundos; cada camada causa 3 pontos por tick de dano, total máximo 9 antes dos modificadores normais de combate.
- Primeiro think em 0,1 segundo; dano a cada pouco mais de 1 segundo por comparação estrita `time > burn_damage_time`; atualização visual a cada 0,02 segundo.
- Na expiração, a camada ainda contribui para o último tick, depois seu bit é removido, como no QC.
- Mantidas as comparações aninhadas originais ao renovar as três camadas ocupadas. Inclusive o caso em que `lifetime1 <= lifetime2` mas `lifetime4 < lifetime1` não renova camada alguma. Não se corrigiu silenciosamente esse comportamento histórico.
- Contágio: centro na origem da vítima + `(0,0,18)`, raio 50, dano `6 + r × 4`, chance estritamente `r > 0,5`. A amostra é compartilhada entre todos os vizinhos do mesmo tick; o crédito do contágio pertence ao jogador em chamas. Dano direto continua creditado ao atacante que iniciou a primeira camada.
- Água acima da cintura extingue no próximo tick de dano; som `player/slimbrn2.wav` e oito bolhas em intervalos 0,1–0,3 segundo. Bolhas sobem inicialmente a 15, usam `s_bubble.spr` e depois a rotina nativa `bubble_bob` do KTX.
- Duas chamas `flame2.mdl`, quadro 1, a 18 unidades acima do jogador e 7 atrás da direção de visão; movimentos opostos de ±2 na frente e ±4 lateralmente. Jogador morto reduz altura em 12. Chama principal produz luz.
- Morte por outra arma extingue as chamas. Morte por Burn mantém o efeito até `DEAD_DEAD`, então remove a chama secundária e anima os quadros 0–5 de `s_explod.spr`, 0,1 segundo por quadro.
- Dor alterna `player/lburn1.wav` / `player/lburn2.wav`, com intervalo mínimo 0,8 segundo; ignição usa `boss1/throw.wav`.

## Adaptações à API KTX

- Campos próprios ficaram em `gedict_t.cfn`; referências nativas usam `EDICT_TO_PROG` / `PROG_TO_EDICT` para `owner` e `enemy`.
- O QC `findradius` entregava uma cadeia. O builtin KTX recebe a entidade inicial da próxima busca; usa-se sua iteração por entidades, sem alterar raio, centro ou filtros.
- Os estados `[frame, próximo_estado]` do QC foram convertidos em callbacks com `nextthink = time + 0.1`.
- Modelos e sons são explicitamente precacheados, incluindo os que o módulo antigo herdava do precache geral de Quake.
- As duas chamas possuem referências verificáveis e limpeza em respawn/disconnect. Referências são anuladas ao fim; a chama secundária recebe classname `burn_flame2` e o gerador de vapor recebe `burn_steam`, permitindo limpeza sem remover outra entidade.
- O projétil Weld e a chama principal guardam o `connect_time` do atacante: se o cliente desconecta ou seu slot passa a outro jogador, dano pendente fica atribuído ao mundo. Respawn normal preserva o crédito porque não muda esse identificador. O gerador de vapor também para caso o slot da vítima seja reutilizado. Esta proteção evita uma atribuição incorreta típica de ponteiros persistentes para slots de cliente.
- O módulo Protect separado do CTFNormal não está ativo no `teamplay 40956` usado pelo servidor de referência. Por isso este porte verifica a invulnerabilidade nativa, sem inventar uma proteção de spawn adicional.

## Contrato de integração com o restante do porte

1. Chamar `CFN_WeldPrecache` e `CFN_BurnPrecache` no precache do modo.
2. Conectar Weld à seleção/munição/cadência original da nailgun.
3. `CFN_Damage` precisa classificar o golpe como `CFN_WEAPON_WELD` / `CFN_WEAPON_BURN`; `T_Damage` deve registrar `cfn.killweapon` antes de disparar a morte do jogador.
4. Em `PainSound`, após os casos de água/lava e antes dos sons de dor comuns: se Burn ativo no jogador, chamar `CFN_BurnPainSound` e retornar.
5. Em morte por Burn: `PlayerDie` escolhe `player_dieb1`; `PlayerDead` executa `GibPlayer`; `VelocityForDamage` parte da velocidade atual e soma aleatoriedade ±80 em X/Y e 50–100 em Z; `ThrowHead` preserva velocidade/altura em vez de aplicar o deslocamento normal de −24.
6. Chamar `CFN_BurnCleanup` em respawn/desconexão, antes de zerar os campos. Não limpar na entrada de `PlayerDie`, pois eliminaria o efeito original da morte.

## Validação executada

Na raiz do repositório, `python scripts/test.py --zig zig` compila os dois arquivos C reais com este harness e os headers do KTX, substituindo a fronteira engine/KTX por funções determinísticas. Requer Python 3.9 ou posterior e Zig 0.13.0 em Windows x64. Os resultados novos ficam em `build/tests/results.json`, junto aos logs. Veja [BUILD](../../docs/BUILD.pt-BR.md).

A suite Weld/Burn passou em **3324 verificações** no estado CFN1. O mesmo comando executa também **252 verificações Shrapnel** e **98 Drone/Hook**, totalizando **3674 verificações unitárias**. A contagem não inclui as verificações históricas da fixture na engine ou a varredura de mapas.

Há 280 combinações de valor aleatório/distância de impacto para validar a fórmula e a chance de incêndio; verificações de voo/luz/vida/quadros; bloqueios de ignição; camadas e renovação; expiração; autoria de contágio; água e oito bolhas; morte normal/Burn; limpeza idempotente; sons e limite de dor. Também são exercitados desconexão do atacante, substituição do jogador no mesmo slot e respawn legítimo com preservação do crédito.

O harness não executa a engine nem simula previsão/rede ou colisão real em mapas. Os testes de entrada de cliente, física e aparência precisam ser feitos no servidor integrado. O dano final (armadura, Quad, regras de times) continua responsabilidade de `T_Damage`. A fixture separada da engine verificou parte dessa integração; seu escopo e limites constam em [CHANGES](../../docs/CHANGES.pt-BR.md).
