# Problema 1 — Tartaruga controlada por mensagens

## Descrição
Implementação de 2 nós (send_msg e turtle). O nó "send_msg" é usado para envio de comandos ao tópico /cmd_turtle, enquanto o nó "turtle" se inscreve nesse tópico, junto com o tópico /turtle1/pose, rotacionando (se necessário) e se deslocando (se possível).

## Requisitos
Para rodar o nó, é necessário ter ROS2 instalado, assim como o pacote python "colcon".

## Como rodar
Primeiro, rode o seguintes comando

``` bash
cd ~/ros2_ws
colcon build --symlink-install
source install/setup.zsh # ou o arquivo setup correspondente ao seu terminal (bash, zsh, etc)
```

No mesmo terminal, rode
```bash
ros2 run turtlesim turtlesim_node&
ros2 run problem_1 send_msg
```

Por último, entre outro terminal, rode
```bash
ros2 run problem_1 turtle
```

## Explicação de lógicas presentes no código

### Nó send_msg

Neste nó, o envio de comandos é feito pela seleção aleatória de 4 strings ("up", "down", "left", "right") a cada 4s.

### Nó turtle

Para o nó turtle foi feita uma máquina de estados ("STANDBY", "ROTATING", "MOVING") que descrevem os estágios de comportamento dele.

Uma função ligada ao timer de 0.05s verifica e chama, se necessário, uma função com base no estado em que ele se encontra.

Obs.: O tempo de 0.05 segundos foi escolhido, pois é o tempo de cada ciclo na frequência 20hz.

À cada recebimento de mensagem pelo tópico /cmd_turtle, a função "command_callback" é chamada e verifica 2 condições: se o estado é STANDBY e se há recebimento da pose do turtlesim_node via tópico /turtle1/pose, se as duas condições forem verdadeiras, o estado é atualizado para ROTATING, o pending_command é atualizado com o recebido pelo tópico e o target_theta é definido pela consulta em um dicionário com base no comando requisitado. Caso o estado não seja STANDBY, a função computa um log warning com a informação de que o comando foi ignorado pelo nó estar ocupado, retornando a função. Em caso de não recebimento da pose do turtlesim_node, um log warning computa essa informação e retorna a função.

Após o timer chamar a função do_rotation ela publica a rotação usando um theta ligado à constante ANGULAR_VEL e calcula o erro de precisão que vem da normalização entre o theta atual e o target_theta. Essa normalização é necessária para que o nó rotacione o mínimo possível, já que uma rotação de +3𝝅/2 tem o mesmo resultado final de −𝝅/2, mesmo que demore mais. Há também uma margem de erro que controla quando o nó deve parar a rotação.

Obs.: Essa ação causa um trade-off de desempenho, caso a margem de erro seja muito baixa, o nó leva mais tempo para concluir a ação de rotação, podendo ignorar comandos por estar ocupado, entretanto, se for (relativamente) alta, o turtlesim_node apresentará inclinações logo nas primeiras iterações.

Com a rotação concluida com base na margem de erro, o x e y de começo da ação do nó são atualizados e se atualiza o estado para MOVING. Logo após, é verificado se as próximas coordenadas são possíveis dentro do espaço da janela do turtlesim, em caso negativo, o nó para, o estado é atualizado para STANDBY e um log warning é computado informando de um "capping" na posição.

Por fim, a velocidade linear é publicada e o turtlesim_node se movimenta até que o trecho percorrido seja equivalente ou maior que 1.0, quando isso ocorre o nó é parado e outra função é chamada, ela atualiza a posição atual e o estado para STANDBY, além de computar um log com informações (posição atual, posição real, theta e comando).
