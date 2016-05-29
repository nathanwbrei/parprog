#include <omp.h>
#include "emsim.h"

void playGroups(team_t* teams) {
 
  static const int cNumTeamsPerGroup = NUMTEAMS / NUMGROUPS; //6
  static const int is[6] = {0,0,0,1,1,2};
  static const int js[6] = {3,2,1,3,2,3};

  int g, i, j, k, goalsI, goalsJ;
 
  #pragma omp parallel for collapse(2) private(i,j,goalsI,goalsJ)
  for (g = 0; g < NUMGROUPS; g++) {
      for (k = 0; k < 6; k++) {
  
          i = g * cNumTeamsPerGroup + is[k];
          j = g * cNumTeamsPerGroup + js[k];

          printf("Thread %d playing group %d game %d: %s vs %s\n", omp_get_thread_num(), g, k, (teams+i)->name, (teams+j)->name);
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

    //#pragma omp critical
    //{
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
    //}
    printf("Thread %d playing final game %d:  %s vs %s\n", omp_get_thread_num(), i, team1->name, team2->name);
  }


  for (i=0; i<numGames;i++) {
      successors[i] = winners[i];
  }
  printf("----------------------\n");
}
