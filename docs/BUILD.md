# Compilar e testar CTF-MPS-KTX

O fluxo validado gera `build/qwprogs.dll`, um módulo nativo Windows x64 para MVDSV. Requer Python 3.9 ou posterior e **Zig 0.13.0**. Não requer pacotes Python adicionais, CMake, Visual Studio, arquivos comerciais do Quake ou um servidor em execução.

Obtenha o compilador pelo [download oficial do Zig 0.13.0](https://ziglang.org/download/0.13.0/zig-windows-x86_64-0.13.0.zip). O SHA-256 do ZIP Windows x64 é `d859994725ef9402381e557c60bb57497215682e355204d754ee3df75ee3c158`, conforme o [índice oficial](https://ziglang.org/download/index.json). Extraia o ZIP e coloque a pasta que contém `zig.exe` no `PATH`.

Na raiz do repositório, execute:

```powershell
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

Se o compilador estiver fora do `PATH`, passe seu executável em `--zig`, entre aspas se o caminho tiver espaços, ou defina a variável de ambiente `ZIG_EXE`. Os scripts verificam a versão antes de compilar. Resolvem os fontes a partir de sua própria localização, portanto o nome da pasta do checkout não é fixo. `--out-dir` permite escolher outro diretório de saída; caminhos relativos nessa opção são resolvidos a partir da pasta do terminal.

## Compilação

`scripts/build.py` lê os arquivos C listados em `source/ktx/CMakeLists.txt` e exclui `bg_lib.c`, que pertence à versão QVM. O alvo é `x86_64-windows-gnu`, com CPU `baseline`, otimização `-O2`, símbolos `-g` e dialeto `gnu17`.

`BOT_SUPPORT=1` acompanha a compilação KTX 1.47 porque essa versão mantém referências internas aos símbolos Frogbot. Isso não habilita bots na configuração do servidor nem inclui o antigo experimento de bots CTF. O módulo de teste em `tests/runtime.c` e a macro `CFN_TEST` não são incluídos nessa DLL.

Saídas:

- `build/qwprogs.dll`: módulo compilado.
- `build/SHA256SUMS.txt`: hash SHA-256 da DLL produzida.
- `build/build-result.json`: versão do compilador, alvo, resultado, hash da DLL e hashes dos fontes e cabeçalhos.
- `build/build-command.json` e `build/build.log`: comando e diagnósticos da compilação.
- `build/zig-cache` e `build/zig-local`: caches locais do compilador.

Os hashes identificam cada compilação; símbolos de depuração, caminho do checkout e metadados do linker podem mudar o hash entre máquinas. Não se afirma reprodução byte a byte.

## Testes

`scripts/test.py` compila e executa três suites em Windows x64:

| Suite | Código exercitado |
| --- | --- |
| Shrapnel | `ctfnormal_shrapnel.c`: tiro, impacto, fragmentos, dano, duração e dono |
| Weld/Burn | `ctfnormal_weld.c` e `ctfnormal_burn.c`: projétil, dano, ignição, camadas e limpeza |
| Drone/Hook | `ctfnormal_drone.c` e `ctfnormal_hook.c`: alvo, orientação, colisões e gancho |

As suites compilam as implementações reais e substituem somente a fronteira KTX/engine por funções determinísticas. A suite Drone/Hook gera uma unidade de compilação em `build/tests/` para incluir o cabeçalho upstream, que não tem proteção contra inclusão repetida, apenas uma vez. Ela utiliza o conteúdo atual dos dois módulos, sem manter uma cópia alternativa da lógica.

Os executáveis, comandos, logs e o resumo `results.json` ficam em `build/tests/`. Falha de compilação ou de qualquer verificação resulta em código de saída diferente de zero. Nenhum script inicia MVDSV, conecta jogadores, instala arquivos em servidores ou altera sua configuração.

Esses testes não substituem uma partida real: não verificam renderização, previsão do cliente, rede, colisões da engine ou equilíbrio entre jogadores. `tests/runtime.c` contém verificações para uma compilação de instrumentação separada, mas não faz parte deste comando nem da DLL distribuída.

## Integração contínua

`.github/workflows/windows.yml` executa os mesmos dois comandos em Windows, depois de baixar e verificar o ZIP oficial do Zig. O workflow publica a DLL e os logs como artefatos da execução. Não publica releases nem implanta um servidor. O histórico do GitHub Actions mostra se uma execução remota efetivamente passou; a presença do arquivo de workflow não comprova uma execução.
