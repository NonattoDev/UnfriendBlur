# UnfriendBlur - design detalhado dos poderes

Este documento define o funcionamento alvo dos poderes. Ele e a referencia para as proximas implementacoes.

## Regras globais

- O jogador pode carregar ate 3 poderes no inventario.
- Pickups na pista entregam poderes aleatorios.
- Se o jogador juntar 3 poderes iguais, eles se fundem automaticamente em 1 Super Power.
- O Super Power ocupa 1 slot especial e consome os 3 poderes normais usados na fusao.
- Se o inventario estiver cheio com 2 poderes iguais e o jogador pegar o terceiro igual, o sistema tambem deve fundir e liberar espaco automaticamente.
- `F` alterna o slot selecionado.
- `B` usa o poder selecionado para frente.
- `N` usa o poder selecionado para tras.
- `G` descarta o poder selecionado no mapa para outro jogador poder pegar.
- Todos os efeitos que mudam resultado de corrida devem ser decididos pelo servidor.
- Vida, dano, velocidade reduzida, escudo, hits e posicao na corrida devem replicar para todos os jogadores.
- Shield pode bloquear poderes ofensivos, exceto se futuramente marcarmos algum poder como "ignora escudo".
- Cada poder precisa ter icone proprio, som, FX visual e feedback claro no HUD.

## Atributos comuns por poder

Cada poder deve ter:

- Tipo (`EUBPowerType`).
- Icone 1024x1024 sem texto.
- Cor principal.
- FX de ativacao.
- FX de impacto, quando aplicavel.
- Som de ativacao.
- Som de impacto, quando aplicavel.
- Dano em vida, se aplicavel.
- Reducao de velocidade, se aplicavel.
- Duracao de efeito, se aplicavel.
- Regras de alvo.
- Regras de bloqueio por Shield.
- Versao Super.

## Regra de Super Power

### Condicao

- 3 poderes normais iguais no inventario viram 1 Super Power.
- Super Powers nao se fundem entre si nesta fase.
- A fusao deve ser clara no HUD, com som, flash e icone dourado.

### Filosofia de design

Super Power nao deve ser apenas "mais dano". Cada Super precisa mudar a decisao de uso:

- Mais alcance.
- Mais area.
- Mais controle.
- Mais risco/recompensa.
- Efeito secundario unico.

### Visual

- Icone proprio com moldura dourada.
- FX mais brilhante e denso.
- Feedback textual/debug por enquanto: `S-BST`, `S-BLT`, etc.
- HUD final deve mostrar um slot especial com brilho/pulso.

## 1. Boost / Nitro

### Fantasia

Impulso agressivo de velocidade, usado para ultrapassar, recuperar depois de batida ou escapar de ataque.

### Entrada

- `B`: impulso para frente.
- `N`: impulso para tras.

### Funcionamento alvo

- Aplica impulso instantaneo no carro.
- O impulso deve respeitar a direcao atual do carro.
- Deve ter rastro visual forte atras do carro.
- Deve ter som curto de motor/turbina.
- Pode causar dano por colisao se a diferenca de velocidade contra o adversario for muito grande.
- O dano de Boost nao e automatico: precisa existir batida real, na direcao do impulso, com velocidade relativa alta.
- Ao bater forte em outro carro:
  - causa dano/impacto no alvo;
  - perde parte da propria velocidade;
  - encerra a janela ofensiva do Boost normal.
- Se a diferenca de velocidade for baixa, deve ser tratada como batida comum de corrida, nao como ataque de poder.

### Parametros iniciais

- Forca base: ajustar por feeling.
- Duracao visual: curta, entre 0.5s e 1.0s.
- Cooldown: nao precisa enquanto o sistema usa inventario consumivel.
- Janela ofensiva: curta, cerca de 1s.
- Velocidade relativa minima: alta.

### Estado atual

- Ja aplica impulso para frente/tras.
- Ja tem FX visual runtime.
- Ja tem janela de batida ofensiva por diferenca de velocidade.
- Boost normal perde bastante velocidade quando acerta.
- Ainda precisa som, dano de vida real e ajuste fino de forca.

### Super Boost

- Consome 3 Boosts.
- Impulso muito mais forte.
- FX mais longo e brilhante.
- Causa impacto/dano maior se bater em adversario com diferenca alta de velocidade.
- Perde bem menos velocidade ao acertar.
- Pode atravessar/acertar mais de um alvo durante a janela curta do Super Boost.
- Nao deve manter 100% da velocidade para sempre; ele conserva o embalo melhor que o Boost normal, mas ainda tem limite.
- Deve futuramente ganhar rastro exclusivo e audio de turbina forte.
- Estado atual: implementado como impulso aumentado, FX Super e ram ofensivo com pouca perda de velocidade.

## 2. Shield

### Fantasia

Protecao temporaria contra ataques.

### Entrada

- `B` ou `N`: ativa o escudo. A direcao nao muda o efeito.

### Funcionamento alvo

- Ativa escudo ao redor do carro por tempo limitado.
- Bloqueia hits de Bolt, Shunt, Mine, Barge e Shock, salvo excecoes futuras.
- Ao bloquear um hit, deve tocar FX de impacto no escudo.
- Deve mostrar claramente que o jogador esta protegido.
- Deve terminar sozinho apos a duracao.

### Parametros iniciais

- Duracao: 5s.
- Bloqueio: 1 ou varios hits durante a duracao. Decisao atual: varios hits enquanto ativo.

### Estado atual

- Ja ativa escudo temporario.
- Ja bloqueia hits recebidos.
- Ja tem FX visual runtime.
- Ainda precisa visual mais colado ao carro e som.

### Super Shield

- Consome 3 Shields.
- Ativa escudo por mais tempo.
- Reflete o primeiro ataque bloqueado de volta para o atacante.
- Depois continua como Shield normal ate acabar.
- Estado atual: reflexao inicial implementada; ainda precisa FX/som de reflexao final.

## 3. Repair

### Fantasia

Recupera vida/dano do carro.

### Entrada

- `B` ou `N`: ativa reparo. A direcao nao muda o efeito.

### Funcionamento alvo

- Recupera parte da vida do carro.
- Nao deve ultrapassar vida maxima.
- Deve ter FX verde/energia reparadora.
- Deve comunicar no HUD quanto recuperou.
- Nao afeta velocidade diretamente.
- Nao afeta outros jogadores.

### Parametros iniciais

- Cura: 25% da vida maxima ou valor fixo equivalente.
- Duracao visual: 1s.

### Estado atual

- Ja usa `UUBVehicleHealthComponent` para recuperar vida real.
- Super Repair recupera mais vida e limpa slow/status negativo via `UUBVehicleStatusComponent`.
- Ja tem FX visual de ativacao.
- Ainda precisa HUD e audio de reparo.

### Super Repair

- Consome 3 Repairs.
- Deve recuperar mais vida.
- Deve limpar efeitos negativos como slow/choque quando criarmos status effects.
- Estado atual: FX Super implementado; cura real depende do sistema de vida.

## 4. Barge

### Fantasia

Explosao curta de impacto ao redor/proximo do carro, boa para afastar adversarios perto.

### Entrada

- `B`: ativa a frente do carro.
- `N`: ativa atras do carro.

### Funcionamento alvo

- Cria onda de impacto curta.
- Empurra carros dentro do raio.
- Causa dano leve/moderado.
- Reduz velocidade por pouco tempo se acertar.
- Deve ser util em briga de posicao, nao como ataque de longo alcance.

### Parametros iniciais

- Raio inicial: 520 Unreal units.
- Dano: baixo/medio.
- Slow: curto, 0.5s a 1.0s.

### Estado atual

- Ja empurra atores dentro do raio.
- Ja respeita frente/tras.
- Ja tem FX visual runtime.
- Ainda falta vida/dano e slow real.

### Super Barge

- Consome 3 Barges.
- Onda de impacto maior e mais forte.
- Deve causar slow mais perceptivel quando o sistema de status existir.
- Estado atual: raio, forca e FX aumentados.

## 5. Bolt

### Fantasia

Disparo reto, rapido, de precisao.

### Entrada

- `B`: dispara para frente.
- `N`: dispara para tras.

### Funcionamento alvo

- Projeteis retos.
- Sem busca automatica.
- Velocidade alta.
- Hitbox apertada no nucleo visual do projetil.
- Ao acertar:
  - causa dano medio;
  - reduz velocidade por curto tempo;
  - aplica impulso leve/medio;
  - toca FX de impacto.

### Parametros iniciais

- Dano: medio.
- Slow: curto, 1s.
- Velocidade: alta.

### Estado atual

- Ja dispara para frente/tras.
- Ja tem hitbox reduzida no nucleo.
- Ja aplica impulso.
- Ja aplica dano e slow curto via componentes de vida/status.
- Ja tem trail/luz/impact FX.
- Ainda precisa HUD/audio e ajuste fino de dano/slow.

### Super Bolt

- Consome 3 Bolts.
- Dispara rajada tripla: esquerda, centro e direita.
- Mantem a fantasia de tiro de precisao, mas abre chance de acertar em curva/disputa.
- Estado atual: rajada tripla implementada.

## 6. Shunt

### Fantasia

Perseguidor agressivo. Ele cria tensao porque o jogador ve/ouve que foi marcado,
mas ainda tem chance real de se defender com habilidade.

### Entrada

- `B`: solta o perseguidor para frente.
- `N`: solta o perseguidor para tras.
- Controle futuro de alvo:
  - teclado: botao dedicado para alternar alvos visiveis;
  - controle: botao dedicado para alternar alvos visiveis;
  - o botao deve percorrer apenas carros dentro da vista/camera do jogador.

### Funcionamento alvo

- Projetil perseguidor com homing.
- Se existe alvo selecionado e ele ainda esta valido, o Shunt persegue esse alvo.
- Se nao existe alvo selecionado:
  - procura carros dentro da vista/camera do jogador;
  - se houver varios carros visiveis, usa o alvo atual escolhido pelo jogador;
  - se nao houver ninguem na vista, mira automaticamente no adversario a frente na corrida.
- "Adversario a frente" nao significa necessariamente na frente fisica do carro; significa
  o proximo colocado acima do jogador no ranking/checkpoints.
- O alvo selecionado precisa ser uma decisao do jogador:
  - se 8 carros aparecem na vista, o jogador pode alternar entre eles;
  - selecionar leva tempo e cria risco, entao nao deve ser instantaneamente perfeito;
  - a UI precisa mostrar qual carro esta marcado sem poluir a tela.
- O Shunt deve ser mais forte que Bolt, mas menos inevitavel que um raio global.
- Deve perseguir de verdade, mas com limites para existir defesa.
- Ao acertar:
  - causa dano alto;
  - capota o carro atingido;
  - pode jogar o carro para cima/frente/lado com impacto forte;
  - aplica slow forte por causa da capotagem e da perda de controle;
  - toca FX de impacto pesado.
- Se o Shunt ja tiver sido acertado por 1 Bolt antes do impacto:
  - perde a forca maxima;
  - acerta com efeito pela metade;
  - causa dano reduzido;
  - nao capota;
  - nao faz o carro voar;
  - faz o carro perder a traseira/estabilidade.

### Defesas e contra-jogo

- Mina para tras:
  - uma mina bem colocada intercepta e destroi o Shunt;
  - a mina tambem explode normalmente no ponto da colisao;
  - isso recompensa leitura de momento e posicionamento.
- Dois tiros de Bolt:
  - o Shunt deve ter vida propria;
  - um Bolt enfraquece o Shunt e corta a forca do impacto pela metade;
  - dois Bolts destroem o perseguidor antes do impacto.
- Shock/raio:
  - raio nao faz efeito no Shunt;
  - o Shunt atravessa a area/queda de raio e continua perseguindo.
- PEM/EMP:
  - quando criarmos esse efeito, ele destroi o Shunt;
  - a destruicao deve gerar pulso/estouro em area;
  - o pulso tambem acerta quem estiver perto do ponto da destruicao.
  - estado atual de prototipo: Barge funciona como pulso/PEM temporario para testar essa defesa.
- Curva no momento exato:
  - uma curva forte no timing certo pode fazer o Shunt perder lock ou passar reto;
  - nao deve ser facil, mas precisa ser possivel para jogador habilidoso.
- Nitro no momento exato:
  - se o jogador usa Boost/Nitro no timing do toque, o Shunt pode errar/overshoot;
  - isso cria defesa ativa, nao apenas defesa por item.
- Shield:
  - bloqueia o impacto se estiver ativo;
  - Super Shield pode refletir ou cancelar com FX especial.

### Parametros iniciais

- Dano: alto.
- Slow: medio, 1.5s a 2.0s.
- Alcance de busca: ajustar por mapa.
- Vida do projetil: 2 pontos de Bolt.
- Dano recebido de Bolt: 1 ponto.
- Janela de curva perfeita: curta, ajustar em playtest.
- Janela de nitro perfeito: curta, ajustar em playtest.
- Impacto cheio: dano alto, capotagem e voo/impulso forte.
- Impacto enfraquecido por 1 Bolt: dano reduzido, perda de traseira/estabilidade,
  sem capotar e sem jogar o carro para o alto.

### Sistemas necessarios

- Sistema de ranking/progresso para saber quem esta a frente.
- Sistema de carros visiveis na camera do jogador.
- Target selector por jogador local:
  - precisa funcionar em split screen;
  - precisa funcionar em online;
  - cada jogador local tem seu alvo independente.
- UI de alvo marcado.
- Projetil com vida/dano recebido.
- Eventos de defesa:
  - destruido por mina;
  - enfraquecido pelo primeiro Bolt;
  - destruido pelo segundo Bolt;
  - ignora Shock/raio;
  - destruido por PEM/EMP com pulso em area;
  - perdeu lock em curva perfeita;
  - perdeu lock por nitro perfeito;
  - bloqueado/refletido por Shield.

### Estado atual

- Ja tem homing simples.
- Ja respeita frente/tras pela direcao de spawn.
- Ja aplica impulso forte.
- Ja aplica dano alto e slow forte no hit cheio.
- Ja aplica dano/slow reduzidos no hit enfraquecido por Bolt.
- Ainda nao tem target selection real.
- Ainda nao sabe quem esta a frente no ranking.
- Ainda nao filtra carros visiveis por camera.
- Ja tem vida propria simples contra Bolt: 1 Bolt enfraquece, 2 Bolts destroem.
- Ja pode ser destruido por mina e por Barge/PEM temporario.
- Ainda falta defesa por curva perfeita e nitro perfeito.
- Falta refinamento da perda de traseira e comportamento de impacto.

### Super Shunt

- Consome 3 Shunts.
- Opcao base: dispara dois perseguidores em angulos abertos.
- Alternativa para testar: dispara um unico perseguidor elite, mais rapido, com mais vida,
  porem ainda defensavel.
- Deve ser assustador, mas nunca 100% garantido.
- Estado atual: dois Shunts homing com spread implementados.

## 7. Mine

### Fantasia

Armadilha deixada na pista.

### Entrada

- `B`: coloca mina na frente.
- `N`: coloca mina atras.

### Funcionamento alvo

- Mina fica armada apos pequeno delay.
- Explode quando carro inimigo entra perto.
- Hitbox deve ser pequena e justa, perto do tamanho visual da mina.
- Ao acertar:
  - causa dano medio/alto;
  - reduz velocidade;
  - aplica impulso para cima/lado;
  - toca FX de explosao.
- Nao deve explodir no dono imediatamente.

### Parametros iniciais

- Delay de armado: 0.6s.
- Hitbox atual alvo: 88 Unreal units.
- Duracao no mapa: 24s.

### Estado atual

- Ja spawna para frente/tras.
- Ja tem delay de armado.
- Ja ignora o dono inicialmente.
- Ja tem hitbox reduzida.
- Ja aplica dano e slow via componentes de vida/status.
- Ainda precisa replicar o estado armado para feedback visual mais correto em clientes.

### Super Mine

- Consome 3 Mines.
- Solta 3 minas em formacao lateral.
- Serve para bloquear faixa, curva ou entrada de atalho.
- Estado atual: formacao tripla implementada.

## 8. Shock

### Fantasia corrigida

Shock e o poder dos raios/trovoes. Nao e uma explosao radial comum.

O icone deve ser um raio/trovao forte, claramente diferente do Bolt.

### Entrada

- `B` ou `N`: ativa o Shock. A direcao do botao nao deve escolher frente/tras local como os outros poderes.

### Funcionamento alvo

- Ao usar Shock, o sistema identifica o jogador em primeiro lugar na corrida.
- O poder cria uma zona de ataque alguns quilometros/unidades de pista a frente do primeiro colocado.
- A zona dispara normalmente 3 raios redondos/sequenciais no caminho do primeiro colocado.
- Os raios devem parecer quedas de trovao/energia vertical, nao projeteis comuns.
- Cada raio deve ter area circular de impacto no chao.
- Os raios devem nascer a frente do primeiro colocado para criar pressao e forcar desvio.
- Se um raio acertar um carro:
  - causa dano;
  - reduz consideravelmente a velocidade;
  - pode causar pequena instabilidade/impulso vertical;
  - toca FX forte de raio/trovao.
- Pode acertar qualquer jogador que passar na area, nao apenas o primeiro colocado.
- Shield deve bloquear o dano/slow se estiver ativo, mas ainda deve mostrar impacto no escudo.

### Regras de alvo

- Alvo principal: jogador em primeiro lugar.
- Se nao houver sistema de corrida/posicao ainda, usar fallback temporario:
  - em single player, spawnar os raios a frente do jogador local;
  - em multiplayer sem ranking, escolher o pawn mais avancado em relacao ao eixo da pista ou ao checkpoint atual.

### Parametros iniciais

- Quantidade de raios: 3.
- Intervalo entre raios: 0.4s a 0.7s.
- Raio circular de cada impacto: medio.
- Distancia a frente do primeiro colocado: depende da escala da pista. Comecar com 1500 a 3000 Unreal units no prototipo e ajustar.
- Dano: medio/alto.
- Slow: forte, 2s a 3s.
- FX: raio vertical, flash, luz azul/branca, decal/anel circular no chao, som de trovao.

### Estado atual

- Atualmente Shock ainda e uma explosao radial deslocada para frente/tras.
- Ja aplica dano e slow forte quando acerta alguem no prototipo radial.
- Isso deve ser substituido pelo comportamento corrigido acima:
  - mirar primeiro colocado;
  - spawnar 3 raios circulares a frente dele;
  - aplicar dano e slow forte se acertar com telegraph visual.

### Super Shock

- Consome 3 Shocks.
- Versao final deve disparar 5 raios em sequencia mirando o primeiro colocado e a area proxima.
- Deve ter aviso visual no chao antes de cada raio para permitir desvio.
- Estado atual: ainda usa o Shock radial temporario, com area/forca/FX aumentados e pulsos extras. A versao correta depende do sistema de ranking/checkpoints.

## Sistemas que ainda precisamos criar

### Vida/dano

Necessario para:

- Repair.
- Bolt.
- Shunt.
- Mine.
- Barge.
- Shock.

Cada carro deve ter:

- Vida maxima.
- Vida atual replicada.
- Evento de dano recebido.
- Evento de reparo.
- Estado destruido/desabilitado ou penalidade ao chegar a zero.

### Slow/reducao de velocidade

Necessario para:

- Bolt.
- Shunt.
- Mine.
- Barge.
- Shock.

Cada slow deve ter:

- Intensidade.
- Duracao.
- Empilhamento ou substituicao por efeito mais forte.
- Replicacao para clientes.
- Feedback visual no carro afetado.

### Posicao de corrida

Necessario para:

- Shock mirar o primeiro colocado.
- HUD de corrida.
- Balanceamento de pickups.

Cada jogador deve ter:

- Checkpoint atual.
- Volta atual.
- Distancia ate o proximo checkpoint.
- Ranking atual replicado.

## Ordem sugerida de implementacao

1. Criar HUD real dos 3 slots e estado do carro.
2. Ajustar controle nativo do Chaos Vehicle antes de reativar qualquer assist.
3. Refinar dano/slow nos hits de Bolt, Shunt, Mine, Barge, Shock e Boost.
4. Criar base de checkpoints/ranking.
5. Refatorar Shock para mirar o primeiro colocado e disparar 3 raios.
6. Criar target selector do Shunt por jogador local.
7. Trocar hotkeys temporarias por Enhanced Input/controle final.
