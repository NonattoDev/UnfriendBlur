# Prototipo de poderes

## Como testar agora

1. Se o Unreal Editor estava aberto antes desta mudanca, reinicie uma vez para carregar as novas `UFUNCTIONs`.
2. Abra `UnfriendBlur.uproject`.
3. Clique em `Play` no mapa atual.
4. O prototipo gera uma pista de combate grande a partir do carro e move o carro para a largada dela: asfalto largo, barreiras laterais, marcacoes, linhas neon, pickups e carros-alvo.
5. Pegue os pickups espalhados pela pista.
6. Use as teclas do teclado:

| Tecla | Acao |
| --- | --- |
| `F` | Alterna entre os poderes nos 3 slots. |
| `B` | Usa/lanca o poder selecionado para frente. |
| `N` | Usa/lanca o poder selecionado para tras. |
| `G` | Descarta o poder selecionado no mapa para outro jogador pegar. |

O inventario tem 3 slots. As teclas `1` a `8` nao adicionam poder; os slots enchem somente ao pegar pickups.

No prototipo atual, o jogo spawna pickups automaticamente ao longo da pista de combate. Eles sao distribuidos em ciclo entre os 8 poderes para existir quantidade suficiente para testar fusoes.

O prototipo tambem spawna varios carros-alvo ao longo da pista. Eles usam a mesh `SM_SportsCar`, tem hitbox fisica e recebem hits dos poderes para testar Bolt, Shunt, Mine, Barge, Shock, Boost e Super Boost.

Se voce juntar 3 poderes iguais, eles viram 1 Super Power automaticamente. Exemplo: 3 Boosts viram 1 Super Boost. Se o inventario estiver cheio com 2 poderes iguais e voce pegar o terceiro igual, a fusao tambem acontece.

Durante o Play, a tela mostra uma linha de debug com os 3 slots. O slot selecionado aparece como `[>BST<]`, `[>BLT<]` etc. Super Powers aparecem como `S-BST`, `S-BLT`, `S-SHK` etc.

## Direcao dos poderes

- Boost: `B` empurra para frente; `N` empurra para tras.
- Boost/Super Boost agora podem causar batida ofensiva se acertarem outro carro com diferenca alta de velocidade. Boost normal perde bastante embalo; Super Boost perde pouco e pode acertar mais de um alvo na janela curta.
- Bolt: `B` atira para frente; `N` atira para tras.
- Shunt: `B` solta perseguidor para frente; `N` solta perseguidor para tras. No prototipo atual o homing ainda e simples; a selecao manual de alvo esta definida no design final.
- Mine: `B` solta na frente; `N` solta atras.
- Barge/Shock: `B` desloca a explosao para frente; `N` desloca para tras.
- Shield/Repair: a direcao nao muda o efeito.

## Observacoes importantes

- A pista gerada e temporaria e serve para teste rapido. Ela ainda nao substitui um `.umap` final com arte feita no Unreal.
- Se voce ainda vir apenas a pista redonda antiga depois de atualizar, feche e reabra o Unreal para carregar a DLL C++ recompilada.
- A pista tem barreiras fisicas, entao ja da para testar batidas, Boost ram, minas e perseguidores em um contexto mais parecido com corrida.
- O assist arcade de controle foi desativado no prototipo para voltarmos ao controle nativo do template e corrigirmos a direcao aos poucos. Detalhes em `Docs/VEHICLE_CONTROL.md`.
- Bolt, Shunt, Mine, Barge e Shock ficam muito mais claros quando existe outro carro/pawn no mapa para ser atingido.
- Os carros-alvo sao apenas bonecos de teste por enquanto: nao dirigem sozinhos, mas recebem impacto fisico e podem ser usados como alvo do Shunt.
- Shunt agora tem protecoes: 1 Bolt enfraquece, 2 Bolts destroem, mina armada destroi e Barge funciona como pulso/PEM temporario. Shock/raio nao afeta o Shunt.
- Shield ja participa da regra: quando um alvo com shield ativo recebe hit, o hit e bloqueado.
- Repair ja recupera vida real usando `UUBVehicleHealthComponent`; Super Repair tambem limpa slow/status negativo.
- Bolt, Boost, Shunt, Mine, Barge e Shock ja aplicam dano/slow basico quando acertam um alvo com componentes do prototipo.
- A linha amarela `Vehicle debug` mostra HP e slow do carro do jogador quando esses componentes existem.
- O componente de poderes e criado automaticamente em runtime para teste, sem precisar editar o Blueprint do carro agora.
- O pickup nao mostra mais letras flutuantes. Ele usa orb/luz colorida e, depois que os PNGs forem importados pelo Unreal, mostra o icone do poder como billboard.
- Ativar poderes agora spawna FX runtime com luz, pulso e sparks orbitando. Impactos de Bolt/Shunt/Mine tambem geram explosao visual.
- Tamanho/hitbox ajustados: pickups usam colisao pequena e visual compacto; projeteis usam colisao no nucleo; minas explodem apenas quando o carro entra perto da mina.
- A esfera redonda grande dos pickups foi removida. O visual atual usa um shard pequeno, luz e icone.

## Icones PNG

Os icones fonte estao em:

`Content/UnfriendBlur/Art/PowerIcons`

Arquivos:

- `Icon_Boost.png`
- `Icon_Shield.png`
- `Icon_Repair.png`
- `Icon_Barge.png`
- `Icon_Bolt.png`
- `Icon_Shunt.png`
- `Icon_Mine.png`
- `Icon_Shock.png`
- `Icon_Super_Boost.png`
- `Icon_Super_Shield.png`
- `Icon_Super_Repair.png`
- `Icon_Super_Barge.png`
- `Icon_Super_Bolt.png`
- `Icon_Super_Shunt.png`
- `Icon_Super_Mine.png`
- `Icon_Super_Shock.png`

Eles sao PNGs 1024x1024 sem texto, prontos para importar no Unreal como `Texture2D` e usar nos pickups/UI dos 3 slots.

## Pickup para pista

Depois da compilacao, a classe `AUBPowerPickup` aparece em C++ Classes. O prototipo ja spawna pickups automaticamente, mas voce tambem pode colocar pickups fixos no mapa.

Para colocar pickup no mapa:

1. Abra o mapa.
2. Procure `UBPowerPickup` no painel de classes/actors.
3. Arraste para a pista.
4. Em `Details`, escolha `FixedPower` se quiser forcar um poder especifico.
5. Se `FixedPower` ficar `None`, o pickup escolhe aleatorio entre os poderes disponiveis.

## Proximo corte

O proximo passo tecnico e criar:

- HUD simples de inventario mostrando os 3 icones.
- Visual de dano/slow no alvo, nao apenas texto debug.
- Transformar a pista gerada em mapa Unreal proprio quando o layout agradar.
- Input real com Enhanced Input em vez das hotkeys temporarias.
- Pickups posicionados na pista.
