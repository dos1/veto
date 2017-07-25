import random
from operator import itemgetter

players = []

for i in range(1, 50):
  players.append( {
    'id': i,
    'score': 0,
    'veto': False,
    'choice': None
  } )

def voting(myVote):
  global players
  forCount = 0
  againstCount = 0
  for player in players:
    player['choice'] = random.choice([False, True, False, True, None])

  players[0]['choice'] = myVote

  for player in players:
    if player['choice']:
      forCount = forCount + 1
    if player['choice'] == False:
      againstCount = againstCount + 1

  print("for: ", forCount)
  print("against: ", againstCount)
  for player in players:
    if player['choice'] == True and forCount >= againstCount:
      player['score'] = player['score'] + 1
    elif player['choice'] == False and forCount <= againstCount:
      player['score'] = player['score'] + 1
    elif player['choice'] != None:
      player['score'] = player['score'] - 1

  sortedPlayers = sorted(players, key=itemgetter('score'), reverse=True)
  print(sortedPlayers[:3])

#print(players)

for i in range(1, 15):
  voting(random.choice([False, True, False, True, None]))
