
setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
source("collectChipResults.R")

rm(list = ls())
setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
source("global.R")

daily1Plot <- oneChipPlot("Voltex60", "Daily_1", 
            c("Thermocouples", "Average Tank Temp", "Input Power", "Output Power"), 
            tmin = 0, tmax = 24)
daily1Plot
ggsave(file = "../graphs/daily1Plot.png", daily1Plot, width = 7, height = 5)


daily1Plot2 <- oneChipPlot("Voltex60", "Daily_1", 
                          c("Thermocouples", "Average Tank Temp", "Input Power", "Output Power"), 
                          tmin = 11, tmax = 16)
daily1Plot2
ggsave(file = "../graphs/daily1Plot2.png", daily1Plot2, width = 7, height = 5)

