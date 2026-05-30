# UnfriendBlur - plano de producao

Este documento organiza o trabalho para transformar o prototipo em um jogo com cara real.

## Direcao de qualidade

UnfriendBlur deve parecer um combat racer arcade premium:

- carros pesados o suficiente para vender impacto, mas responsivos;
- poderes com leitura instantanea na pista;
- FX fortes, com luz, rastro, impacto e audio;
- HUD limpo, por jogador, preparado para split screen;
- multiplayer planejado desde o inicio, mesmo quando o teste e single player;
- nenhuma regra importante decidida apenas no cliente.

## Corte vertical atual

Antes de expandir conteudo, o objetivo e fazer um pequeno loop parecer jogo:

1. dirigir com controle confiavel;
2. pegar pickup na pista;
3. ver o poder nos 3 slots;
4. usar Bolt/Shunt/Mine/Boost contra outro carro;
5. aplicar dano/slow/impacto real;
6. ver feedback visual claro no alvo;
7. juntar 3 poderes iguais e usar uma versao Super.

## Agentes paralelos

### Arte e visual

Responsavel por:

- linguagem visual dos poderes;
- pickups e icones;
- FX de ativacao e impacto;
- pista, luz, camera e acabamento visual;
- riscos de assets Unreal e Git LFS.

### Gameplay e veiculo

Responsavel por:

- controle arcade do carro;
- drift;
- estabilidade, capotagem e laterais da pista;
- input de teclado/controle;
- caminho seguro sem brigar com Chaos Vehicle.

### Poderes e combate

Responsavel por:

- prioridade dos poderes;
- vida, dano, slow e status effects;
- counterplay de Shunt;
- Shock corrigido como raio no primeiro colocado;
- versoes Super com diferencas reais.

### Arquitetura multiplayer

Responsavel por:

- autoridade do servidor;
- replicacao de pickups, inventario e hits;
- split screen + online misto;
- separar input/UI por jogador local;
- evitar dependencias globais em Player 0.

## Prioridade tecnica

### 1. Base de dano e status

Estado atual: implementado em C++.

- `UUBVehicleHealthComponent`;
- `UUBVehicleStatusComponent`;
- eventos replicados de dano, cura, slow e escudo.

Bolt, Boost, Shunt, Mine, Barge e Shock ja aplicam dano/slow basico. Repair ja cura vida real e Super Repair limpa slow/status.

Proximo ajuste desta frente:

- balancear valores;
- mostrar vida/status no HUD;
- criar feedback visual no carro atingido;
- decidir o que acontece quando a vida chega a zero.

### 2. HUD de inventario

Criar HUD simples por jogador:

- 3 slots com icones;
- slot selecionado;
- destaque de Super Power;
- feedback de pickup/fusao/uso.

Motivo: o jogo precisa parar de depender de texto debug para parecer jogavel.

### 3. Controle do carro

Seguir em etapas:

- diagnosticar se `A/D` chegam ao controller;
- ajustar input/Blueprint/Chaos Vehicle antes de qualquer assist fisico;
- adicionar drift e estabilidade por pequenas mudancas testaveis;
- nunca sobrescrever velocidade linear/angular todo tick para corrigir curva.

### 4. Visual real dos poderes

Comecar por 3 poderes que vendem o jogo:

- Boost: rastro forte e impacto ofensivo;
- Bolt: tiro rapido com trail e impacto legivel;
- Shunt: perseguidor dramatico com aviso, defesa e impacto pesado.

Depois:

- Mine;
- Shield;
- Shock correto com raios;
- Barge;
- Repair.

## Regra de implementacao

Cada mudanca deve passar por:

1. compilar;
2. testar em Play;
3. confirmar que nao quebrou controle;
4. registrar no doc relevante;
5. commit pequeno;
6. push para `main`.

## O que evitar

- editar `.uasset` manualmente fora do Unreal;
- misturar ajuste de controle com mudanca grande de poderes;
- criar sistema client-only para coisa que altera corrida;
- depender de `Player 0` para UI/input final;
- comprar/adotar pacote grande antes de saber o feeling alvo;
- continuar adicionando poderes sem vida/status/ranking.

## Sintese dos agentes

- Arte/visual: duplicar o mapa para `Content/UnfriendBlur/Maps` antes de um passe visual pesado; priorizar pickup premium, HUD real, camera e FX por poder.
- Veiculo: manter o assist antigo desligado; primeiro confirmar input e ajustar Chaos Vehicle/Blueprints nativos.
- Poderes: priorizar Bolt, Boost e Shunt porque eles vendem o combate rapidamente.
- Multiplayer: remover gradualmente RPCs de debug, evitar `Player 0`, e tratar cada jogador local como participante proprio com `PlayerState`.
