#include <omp.h>
#include "emsim.h"

// play all (36) group games
void playGroups(team_t* teams) {
  printf("Called playGroups\n");
  static const int cNumTeamsPerGroup = NUMTEAMS / NUMGROUPS;
  int g, i, j, goalsI, goalsJ;

  for (g = 0; g < NUMGROUPS; ++g) {
    for (i =  g * cNumTeamsPerGroup; i < (g+1) * cNumTeamsPerGroup; ++i) {
      #pragma omp parallel for private(goalsI, goalsJ) firstprivate(g,i)
      for (j = (g+1) * cNumTeamsPerGroup - 1; j > i; --j) {

        printf("Thread %d started playing %d,%d,%d: %s vs %s\n", omp_get_thread_num(), g,i,j, (teams+i)->name, (teams+j)->name);
        // team i plays against team j in group g
        playGroupMatch(g, teams + i, teams + j, &goalsI, &goalsJ);


        #pragma omp critical
        {
          teams[i].goals += goalsI - goalsJ;
          teams[j].goals += goalsJ - goalsI;
          if (goalsI > goalsJ)
            teams[i].points += 3;
          else if (goalsI < goalsJ)
            teams[j].points += 3;
          else {
            teams[i].points += 1;
            teams[j].points += 1;
          }
        }
        printf("Thread %d finished playing %d,%d,%d: %s vs %s\n", omp_get_thread_num(), g,i,j, (teams+i)->name, (teams+j)->name);
      }
    }
  }
}

// play a specific final round 
void playFinalRound(int numGames, team_t** teams, team_t** successors){
  team_t* team1;
  team_t* team2;
  int i, goals1 = 0, goals2 = 0;

  team_t * winners[8];

  #pragma omp parallel for private(goals1, goals2, team1, team2) 
  for (i = 0; i < numGames; ++i) {
    team1 = teams[i*2];
    team2 = teams[i*2+1];
    playFinalMatch(numGames, i, team1, team2, &goals1, &goals2);

    #pragma omp critical
    {
      if (goals1 > goals2)
        winners[i] = team1;
      else if (goals1 < goals2)
        winners[i] = team2;
      else {
        playPenalty(team1, team2, &goals1, &goals2);
        if (goals1 > goals2)
          winners[i] = team1;
        else
          winners[i] = team2;
      }
    }
    printf("Thread %d played final match %d=>  %s-%d : %s-%d\n", omp_get_thread_num(), i, team1->name, goals1, team2->name, goals2);
  }


  for (i=0; i<numGames;i++) {
      successors[i] = winners[i];
  }
  printf("----------------------\n");
}
