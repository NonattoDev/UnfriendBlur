# UnfriendBlur - controle arcade do carro

Este documento registra o ajuste rapido de dirigibilidade do prototipo.

## Problemas observados

- Carro pesado para controlar.
- Capotagem muito facil em impactos pequenos.
- Drift quase inexistente.
- Lombada/lateral da pista joga o carro para o alto com facilidade.

## Estado atual

O componente `UUBArcadeVehicleAssistComponent` existe, mas nao e mais adicionado automaticamente no Play.

Motivo: o assist interferiu demais na direcao. Voltamos ao controle nativo do template para corrigir o carro aos poucos.

O prototipo agora mostra uma linha amarela `Vehicle debug` durante o Play. Ela nao muda fisica nem input; serve so para diagnostico.

Essa linha mostra:

- classe do Pawn atual;
- teclas `W`, `A`, `S`, `D`, `Space` e `Shift` chegando ao PlayerController;
- valor bruto de direcao por `A/D`;
- velocidade aproximada em km/h;
- yaw atual do carro.

No log, o prototipo tambem registra os componentes de movimento/veiculo encontrados no Pawn.

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
- nao sobrescrever velocidade angular do carro a cada tick;
- primeiro ajustar input/Blueprint/Chaos Vehicle Movement;
- depois adicionar assist fisico pequeno, isolado e testado um por vez.

## O que o assist fazia

- Downforce por velocidade para manter o carro mais grudado no chao.
- Anti-flip para reduzir capotagens acidentais.
- Amortecimento de roll/pitch no ar.
- Limitador de lancamento vertical perto do chao, principalmente em laterais/lombadas.
- Amortecimento ao bater em parede/lateral da pista.
- Assist de direcao removido do prototipo rapido para nao brigar com o controle nativo do carro.
- Curva arcade adicionada: quando A/D vira a roda, a direcao da velocidade tambem acompanha levemente a curva.
- A curva arcade rotaciona a velocidade do carro, sem empurrar lateralmente como no ajuste anterior.
- Drift assist somente com `Space` + direcao.
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
