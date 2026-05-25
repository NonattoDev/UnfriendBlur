# UnfriendBlur

UnfriendBlur e um prototipo de corrida arcade com combate veicular, poderes na pista, inventario de 3 slots e foco em multiplayer local/online desde a arquitetura.

O projeto nasce inspirado no sentimento de party racing agressivo de jogos como Blur, mas com uma diferenca central: juntar 3 poderes iguais cria um Super Power, mudando o ritmo da corrida e abrindo jogadas mais fortes, arriscadas e visualmente pesadas.

> Projeto em desenvolvimento. A base atual e um prototipo jogavel em Unreal Engine, ainda sem representacao de produto final.

## Visao

A meta e construir um combat racer moderno para jogar com amigos:

- LAN e online multiplayer.
- Split screen para ate 4 jogadores.
- Mistura dos dois modos: por exemplo, 2 pessoas no mesmo PC entrando em uma partida online.
- Poderes espalhados pela pista, sem ganhar item por atalho de teclado.
- Inventario com 3 slots.
- Super Powers criados automaticamente ao juntar 3 poderes iguais.
- Visual de poderes forte, claro e pesado.
- Carros com fisica arcade: rapido, agressivo, controlavel e divertido.

## Estado atual

Ja existe uma base C++ dentro do projeto Unreal com:

- Sistema de inventario de poderes com 3 slots.
- Pickups aleatorios espalhados pelo mapa ao iniciar o Play.
- Fusao automatica de 3 poderes iguais em 1 Super Power.
- Uso de poder para frente e para tras.
- Descarte de poder no mapa para outro jogador poder pegar.
- FX runtime para ativacao e impacto.
- Icones fonte dos poderes em PNG.
- Carros-alvo de teste para acertar poderes.
- Regras iniciais de defesa do Shunt.
- Boost e Super Boost com colisao ofensiva por diferenca alta de velocidade.
- Documentacao de arquitetura e design dos poderes.

## Controles atuais

| Tecla | Acao |
| --- | --- |
| `W` | Acelerar, pelo controle atual do template Unreal. |
| `A` / `D` | Direcao, pelo controle atual do template Unreal. |
| `S` | Freio/re. |
| `F` | Alterna o slot selecionado. |
| `B` | Usa/lanca o poder selecionado para frente. |
| `N` | Usa/lanca o poder selecionado para tras. |
| `G` | Descarta o poder selecionado no mapa. |

As teclas `1` a `8` nao adicionam poderes ao inventario. Os poderes devem ser coletados no mapa.

## Poderes

O prototipo trabalha com 8 poderes base:

- Boost
- Shield
- Repair
- Barge
- Bolt
- Shunt
- Mine
- Shock

Cada poder tera uma versao Super. A regra base ja existe: 3 poderes iguais viram 1 Super Power.

Exemplos:

- 3 Boosts viram Super Boost.
- 3 Bolts viram Super Bolt.
- 3 Shunts viram Super Shunt.

## Como testar

1. Instale/abra com Unreal Engine 5.7.
2. Garanta que o Git LFS esta instalado antes de clonar ou baixar o projeto.
3. Abra `UnfriendBlur.uproject`.
4. Compile o projeto se o Unreal pedir.
5. Abra o mapa configurado do template de veiculo.
6. Clique em `Play`.
7. Pegue pickups no mapa e teste os poderes contra os carros-alvo.

## Git LFS

Este projeto usa Git LFS para assets Unreal e arquivos de arte.

Antes de clonar:

```powershell
git lfs install
```

Depois de clonar:

```powershell
git lfs pull
```

Sem LFS, arquivos `.uasset`, `.umap`, `.fbx` e imagens podem vir apenas como ponteiros e o projeto nao abrira corretamente.

## Estrutura importante

- `Source/UnfriendBlur`: codigo C++ do prototipo.
- `Content/UnfriendBlur/Art/PowerIcons`: icones fonte dos poderes.
- `Docs/POWER_DESIGN.md`: funcionamento detalhado dos poderes.
- `Docs/POWER_PROTOTYPE.md`: como testar o estado atual.
- `Docs/ARCHITECTURE.md`: decisoes de arquitetura.
- `Docs/VEHICLE_CONTROL.md`: notas sobre controle e fisica do carro.

## Roadmap proximo

- Corrigir e refinar controle arcade do carro sem quebrar o Chaos Vehicle.
- Criar vida/dano real dos veiculos.
- Criar status effects: slow, stun, estabilidade e recuperacao.
- Refatorar Shock para mirar o primeiro colocado com raios na pista.
- Implementar selecao de alvo do Shunt.
- Criar HUD real dos 3 slots com icones.
- Preparar input final para teclado e controle.
- Evoluir suporte a split screen e multiplayer desde a base.

## Nota legal

UnfriendBlur e um projeto independente de prototipo e aprendizado. Nao e afiliado, licenciado ou endossado por Blur ou seus detentores de direitos.
