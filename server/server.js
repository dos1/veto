/* PROTOCOL
 broadcasted events:
   S - game starts
   V - voting starts
   C{1} - voting counter, {1} secs left
   F{1} - {1} players voted for
   A{1} - {1} players voted against
   N{1} - {1} players didn't vote
   E{1} - voting ended; {1} is a result: either F (for) or A (against)
      
   v{1} - player with nick {1} used veto
   
 events sent only to monitor:
   P{1} - currently {1} players are connected

   J{1} - player with nick {1} joined
   L{1} - player with nick {1} left
   R{1} - player with nick {1} reconnected

 events sent only to players:
   cookie:{1} - cookie for reconnecting
   nick:{1} - nick sent after reconnecting
   end - this player has ended their game
   score:{1} - current score of the player
   TODO: canVeto - player can veto in this round
   err - if reconnection didn't succeed
   ok - if reconnection did succeed
   
--------------------------------------------------
monitor can send:
  {"type": "monitor"} - to become a monitor (there can be only one)
  {"type": "start"} - to start the game
  {"type": "voting"} - to start the voting
player can send:
  {"type": "join", "nick": string} - to join
  {"type": "reconnect", "cookie": string} - to reconnect
  {"type": "vote", "choice": choice} - to vote. can be changed before the voting ends; choice: true/false/null
  {"type": "veto"} - to veto.
  
debug:
  TODO: {"type": "debug"} - returns data of all players and state
  
TODO: test reconnect, start vs. voting; implement veto and debug monitor
*/

const WebSocket = require('ws');
const randomstring = require('randomstring');

const wss = new WebSocket.Server({ port: 8889 });

let players = [];

let round = 0;

let canVeto = false;

let state = {
  monitor: null,
  voting: false,
  counter: null,
  started: false
};

let countVotes = function() {
    let allplayers = 0;
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        if (client.data) {
          allplayers++;
        }
      }
    });


    let forvotes = 0; let against = 0;
    players.forEach(function(player) {
      if (player.vote == true) {
        forvotes++;
      }
      if (player.vote == false) {
        against++;
      }
    });
    let nonevotes = allplayers - forvotes - against;

    wss.broadcast('F' + forvotes);
    wss.broadcast('A' + against);
    wss.broadcast('N' + nonevotes);
 
    let result = 'A';
    if (forvotes > against) {
      result = 'F'
    }

    wss.broadcast('E' + result);

    players.forEach(function(player) {
      if (player.vote == (result == 'F')) { // points for voting like majority
        player.score++;
      } else if (player.vote != null) {
        player.score--;
      }
      if (player.ws.readyState == WebSocket.OPEN) {
        player.ws.send('score:'+player.score);
      }

    });
    
    round++;

};

let startGame = function() {
  if (state.started) {
    console.log('RESTART');
  } else {
    console.log('START');
  }
  state.started = true;
  wss.broadcast('S');
  
  state.counter = 0;
  state.voting = false;
  
  players.forEach(function(player) {
      player.vote = null;
      player.vetoRight = false;
      player.score = 0;
      player.ended = false;
  });
  
  round=0;
};

let startVote = function() {
    
    if (state.voting) return;
    
    console.log('VOTE round ' + round);

  players.forEach(function(player) {
    player.vote = null;
  });

  state.voting = true;
  wss.broadcast('V');

  state.counter = 10;

  var tick = function() {
    if (state.counter >= 0) {
      wss.broadcast('C' + state.counter);
      state.counter--;
      setTimeout(tick, 1000);
    } else {
      state.voting = false;
      countVotes();
    }
  };

  tick();
};

// Broadcast to all.
wss.broadcast = function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};

wss.on('connection', function connection(ws) {

  //wss.broadcast("connected");

  ws.on('close', function() {
    if (ws==state.monitor) {
      state.monitor = null;
      return;
    }
    if (!ws.data) return;
    let playerCount = 0;
    ws.data.connected = false;
    wss.clients.forEach(function each(client) {
      if (client.readyState === WebSocket.OPEN) {
        if ((client.data) && (client.data.connected)) {
          playerCount++;
        }
      }
    });
    if (state.monitor) {
      state.monitor.send('P' + playerCount);
      state.monitor.send('L' + ws.data.name);
    }
  });

  ws.on('message', function incoming(data) {
    // Broadcast to everyone.
    //wss.broadcast(data);

    console.log('got ' + data + ' from ' + ws.data)

    //if (data.startsWith('{')) {
    try {
      data = JSON.parse(data);
    } catch(e) {
      console.log('error parsing ' + data + ' - ' + ws.data)
      return;
    }
    //}

    if (data.type == 'monitor') {
      state.monitor = ws;
      console.log('monitor registered');
      
     let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
    state.monitor.send('P' + playerCount);

    }

    if (data.type == 'join') {
      if (ws.data) return;
      console.log(data.nick + ' joined');
      ws.data = {
        name: data.nick,
        score: 0,
        vote: null,
        vetoRight: false,
        ended: false,
        cookie: randomstring.generate(),
        connected: true,
        ws: ws
      };
      players.push(ws.data);

      let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
      if (state.monitor) {
        state.monitor.send('P' + playerCount);
        state.monitor.send('J' + data.nick);
      }
      ws.send('cookie:' + ws.data.cookie);
    }

    if (data.type == 'reconnect') {
      // TODO: FIXME: search for cookie and reattach
      players.forEach(function(player) {
        if (data.cookie == player.cookie) {
          ws.data = player;
        }
      });
      
      if (!ws.data) {
          ws.send('err');
          return;
      } else {
          ws.send('ok');
      }
      ws.data.connected = true;
      ws.data.ws = ws;
        
      let playerCount = 0;
      wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
          if ((client.data) && (client.data.connected)) {
            playerCount++;
          }
        }
      });
      if (state.monitor) {
        state.monitor.send('P' + playerCount);
        state.monitor.send('R' + ws.data.name);
      }
      ws.send('score:' + ws.data.score);
      ws.send('nick:' + ws.data.name);
      if (ws.data.ended) ws.send('end');
    }

    if (data.type == 'vote') {
      if (!ws.data) return;
      if (ws.data.ended) return;
      if (state.voting) {
        console.log(ws.data.name + ' voted ' + data.choice)
        ws.data.vote = data.choice;
      }
    }

    if (data.type == 'veto') {
      if (!ws.data) return;
      if (state.voting) {
        if ((ws.data.vetoRight) && (canVeto)) {
          canVeto = false;
          console.log(ws.data.name + ' said VETO!')
          ws.data.ended = true;
          wss.broadcast('v' + ws.data.name);
          ws.send('end');
        }
      }
    }

    if (data.type == 'start') {
      if (ws == state.monitor) {
        startGame();
      }
    }
    if (data.type == 'voting') {
      if (ws == state.monitor) {
        startVote();
      }
    }

  });
});
