# UnfriendBlur - controle arcade do carro

Este documento registra o ajuste rapido de dirigibilidade do prototipo.

## Problemas observados

- Carro pesado para controlar.
- Capotagem muito facil em impactos pequenos.
- Drift quase inexistente.
- Lombada/lateral da pista joga o carro para o alto com facilidade.

## Estado atual

O componente `UUBArcadeVehicleAssistComponent` voltou a ser adicionado automaticamente no Play, mas agora com tres regras:

- o input de `W/A/S/D` e enviado diretamente para o `ChaosVehicleMovementComponent`;
- enquanto o Blueprint do carro nao responde bem sozinho, existe um fallback fisico controlado para aceleracao e curva;
- qualquer assist fisico precisa ter limite de velocidade/forca para nao explodir a simulacao;
- o spawn do carro no prototipo usa clearance baixo para nao comecar caindo/flutuando;
- o carro so e considerado apoiado quando esta perto do solo; ver o chao a distancia nao basta;
- `Space` nao aciona mais handbrake fisico do Chaos enquanto a suspensao/base do carro nao estiver estavel;
- se o carro nascer alguns centimetros acima do ideal, um assist leve faz ele assentar sem depender do jogador apertar `Space`;
- toda rodada de teste do jogador local grava telemetria em `Saved/HandlingTelemetry.csv`.

O prototipo tambem mostra uma linha amarela `Vehicle debug` durante o Play. Ela serve para diagnostico rapido do input e do estado do carro.

Essa linha mostra:

- classe do Pawn atual;
- teclas `W`, `A`, `S`, `D`, `Space` e `Shift` chegando ao PlayerController;
- valor bruto de direcao por `A/D`;
- velocidade aproximada em km/h;
- yaw atual do carro.

No log, o prototipo tambem registra os componentes de movimento/veiculo encontrados no Pawn e o caminho da telemetria.

## Como testar de forma realista

1. Abrir o projeto e jogar normalmente por 2 a 3 minutos.
2. Forcar curvas fechadas, batidas leves na lateral da pista e retomada de velocidade.
3. Fechar o Play.
4. Analisar `Saved/HandlingTelemetry.csv`.

O CSV registra:

- velocidade total, velocidade frontal e velocidade lateral em km/h;
- input de direcao, acelerador, freio e drift;
- contato com o chao e distancia ate o solo;
- angulo de derrapagem;
- yaw rate do ator;
- roll, pitch e velocidade vertical.

Esse arquivo passa a ser a fonte principal para corrigir dirigibilidade. Se o carro parecer pesado, travado, capotar facil ou nao apontar para a curva, o proximo ajuste deve partir desses numeros.

## Diagnostico do que quebrou

O erro foi tentar corrigir dirigibilidade por fora do sistema de veiculo, mexendo diretamente na fisica do corpo do carro.

As partes perigosas foram:

- `SetPhysicsLinearVelocity`: sobrescrevia a direcao da velocidade que o Chaos Vehicle estava calculando.
- `SetPhysicsAngularVelocityInRadians`: sobrescrevia parte da rotacao/yaw/roll do carro.
- `AddForce` lateral para drift: empurrava o carro de lado em vez de deixar pneu/suspensao gerar curva.
- `AddTorque` de direcao: brigava com a direcao nativa do veiculo.
- leitura manual de `A/D/Space/Shift`: ficava separada do Enhanced Input/Blueprint original.

Resultado: a roda visual podia virar, mas o corpo fisico/velocidade do carro nao obedecia como o template esperava.

Se a direcao ainda falhar, olhar primeiro a linha `Vehicle debug`:

- se `A/D` nao mudam de `0` para `1`, o problema esta no input/viewport;
- se `A/D` mudam, mas o carro nao gira, o problema esta no Blueprint/Chaos Vehicle/wheel setup;
- se yaw muda, mas a trajetoria nao muda, ainda existe conflito de fisica/velocidade ou pneu sem aderencia lateral suficiente.

Regra daqui para frente:

- nao alterar velocidade linear do carro a cada tick para corrigir curva;
- nao teletransportar a rotacao do ator para corrigir curva;
- nao usar `AddForce` sem telemetria, limite e comparacao de resultado;
- primeiro ajustar input/Blueprint/Chaos Vehicle Movement;
- depois adicionar assist fisico pequeno, isolado e testado um por vez.

## O que o assist faz agora

- Downforce por velocidade para manter o carro mais grudado no chao.
- Anti-flip para reduzir capotagens acidentais.
- Amortecimento de roll/pitch no ar.
- Limitador de lancamento vertical perto do chao, principalmente em laterais/lombadas.
- Amortecimento ao bater em parede/lateral da pista.
- Fallback de aceleracao por `AddForce`, limitado por velocidade maxima.
- Assist leve de curva por velocidade/angular velocity, sem teletransportar rotacao.
- Grip lateral normal para impedir que o carro continue atravessado quando nao esta em drift.
- Damping/cap de yaw para impedir o carro de continuar girando depois que o jogador solta A/D.
- Clamp de velocidade planar, lateral e vertical somente quando o carro passa dos limites de seguranca.
- Limitador especifico de velocidade no ar para impedir o carro de virar missil depois de uma lombada/borda.
- Temporariamente, `Space` nao aciona handbrake. O drift sera refeito depois que o carro base parar de flutuar/voar.
- Telemetria de handling para comparar ajuste por ajuste.
- `Shift` fica livre para virar olhar para tras quando conectarmos a camera/input final.

## Filosofia

Isto nao substitui o ajuste final do veiculo Chaos/Blueprint.

E uma camada arcade rapida para deixar o prototipo jogavel enquanto criamos poderes,
multiplayer, split screen e combate.

Mais tarde devemos ajustar:

- centro de massa do carro;
- massa e damping;
- pneus/friction curves;
- torque e caixa de marcha;
- forca de freio;
- handbrake;
- colisao das laterais da pista;
- recuperacao pos-capotagem;
- camera em drift/boost.
- camera de olhar para tras no `Shift`.

## Pacotes prontos

Existem pacotes/plugins prontos para veiculo arcade, drift e corrida no Unreal.
Eles podem acelerar, mas antes de comprar/adotar um pacote grande precisamos decidir:

- se vamos manter Chaos Vehicles;
- se queremos fisica arcade propria;
- se drift deve ser realista, semi-arcade ou estilo Blur;
- se o pacote suporta multiplayer;
- se o pacote suporta split screen local;
- se permite customizar powers/combat sem brigar com o framework.

Minha recomendacao atual: continuar com assist proprio no prototipo e so avaliar pacote
quando o feeling alvo estiver mais claro.
