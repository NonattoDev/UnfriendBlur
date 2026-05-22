# UnfriendBlur - arquitetura inicial

## Objetivo

UnfriendBlur e um arcade combat racer multiplayer: corrida rapida, carros com direcao simples de sentir, poderes ofensivos/defensivos e partidas com amigos.

O projeto deve nascer suportando:

- Split screen local ate 4 jogadores.
- LAN multiplayer.
- Online multiplayer.
- Modo misto: mais de um jogador local no mesmo PC entrando em uma partida de rede.

## Principio central

Tudo que muda o resultado da corrida precisa ser decidido pelo servidor:

- Quem pegou um poder.
- Qual poder foi usado.
- Quem foi atingido.
- Quem completou checkpoint/volta.
- Resultado da corrida.

Clientes podem prever efeitos visuais e input, mas a verdade do jogo fica no servidor. Isso vale tambem para split screen dentro de partida online: cada jogador local tem seu proprio `PlayerController`, mas a autoridade continua no servidor.

## Camadas

### Session layer

Responsavel por criar, buscar e entrar em partidas.

Primeira fase:

- `OnlineSubsystemNull` para LAN.
- Entrada direta por IP para testes.

Fase online:

- Steam ou Epic Online Services.
- Mesma interface de jogo por cima, trocando apenas o backend de sessoes.

### Race layer

Responsavel por corrida:

- Grid de largada.
- Contagem regressiva.
- Checkpoints.
- Voltas.
- Posicao na corrida.
- Fim de corrida.

Classes previstas:

- `GameMode`: regras da corrida no servidor.
- `GameState`: estado replicado da corrida.
- `PlayerState`: nome, posicao, voltas, tempo, carro escolhido.
- `Checkpoint Actor`: volume de passagem com ordem.

### Vehicle layer

Responsavel pelo carro:

- Pawn do veiculo.
- Movimento via Chaos Vehicles.
- Input local.
- Replicacao do movimento.
- Ajustes arcade de direcao, peso, tracao e drift.

No inicio, os Blueprints do template de veiculo continuam sendo usados. C++ entra para sistemas compartilhados e replicados.

### Power layer

Responsavel pelos poderes inspirados nos combat racers classicos:

- Boost/Nitro.
- Shield.
- Repair.
- Barge.
- Bolt.
- Shunt.
- Mine.
- Shock.

Classes iniciais:

- `UUBPowerInventoryComponent`: fica no carro e guarda/usa poderes.
- `AUBPowerProjectile`: projeteis como Bolt e Shunt.
- `AUBPowerMine`: mina deixada na pista.
- `AUBPowerPickup`: pickup que pode ser colocado no mapa.
- `UUBPowerPrototypeSubsystem`: hotkeys temporarias para testar poderes sem editar Blueprint.

## Split screen + rede

Para suportar dois jogadores no mesmo PC dentro de uma partida online:

- Cada jogador local precisa de um `LocalPlayer`.
- Cada `LocalPlayer` tem seu proprio `PlayerController`.
- O servidor enxerga cada jogador como participante separado.
- UI, input e camera precisam ser por jogador, nao globais.

Regra pratica: nada importante deve depender de "o primeiro jogador" ou "Player 0", exceto ferramentas temporarias de debug.

## Prototipo atual

O primeiro corte tecnico adiciona um modulo C++ leve e um sistema de poderes testavel em runtime.

Durante o Play-In-Editor, hotkeys temporarias ativam poderes no carro controlado pelo jogador local:

- `1`: Boost/Nitro.
- `2`: Shield.
- `3`: Repair.
- `4`: Barge.
- `5`: Bolt.
- `6`: Shunt.
- `7`: Mine.
- `8`: Shock.

Essas hotkeys existem para acelerar validacao. Quando a UI/input final entrar, elas devem virar acoes reais de controle.

## Proximos marcos

1. Validar que o projeto C++ compila.
2. Testar os poderes no mapa atual.
3. Ajustar valores de forca, raio, velocidade e duracao ate ficar divertido.
4. Colocar pickups reais na pista.
5. Criar base de corrida: checkpoints e voltas.
6. Criar LAN host/join.
7. Adicionar segundo jogador local no split screen.
8. Testar modo misto: dois locais + outro PC na LAN.
