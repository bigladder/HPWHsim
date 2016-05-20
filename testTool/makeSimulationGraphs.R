library(reshape2)
library(ggplot2)

setwd("./tests")

simData4_1 <- read.csv("./DrawProfileTest_4p1_24hr67/TestToolOutput.csv")
attr(simData4_1, "source")  <- "4 person - Day 1"
simData4_2 <- read.csv("./DrawProfileTest_4p2_24hr67/TestToolOutput.csv")
attr(simData4_2, "source")  <- "4 person - Day 2"
simData4_3 <- read.csv("./DrawProfileTest_4p3_24hr67/TestToolOutput.csv")
attr(simData4_3, "source")  <- "4 person - Day 3"
simData4_4 <- read.csv("./DrawProfileTest_4p4_24hr67/TestToolOutput.csv")
attr(simData4_4, "source")  <- "4 person - Day 4"
simData4_5 <- read.csv("./DrawProfileTest_4p5_24hr67/TestToolOutput.csv")
attr(simData4_5, "source")  <- "4 person - Day 5"
simData4_6 <- read.csv("./DrawProfileTest_4p6_24hr67/TestToolOutput.csv")
attr(simData4_6, "source")  <- "4 person - Day 6"
simData4_7 <- read.csv("./DrawProfileTest_4p7_24hr67/TestToolOutput.csv")
attr(simData4_7, "source")  <- "4 person - Day 7"

simData5_1 <- read.csv("./DrawProfileTest_5p1_24hr67/TestToolOutput.csv")
attr(simData5_1, "source")  <- "5 person - Day 1"
simData5_2 <- read.csv("./DrawProfileTest_5p2_24hr67/TestToolOutput.csv")
attr(simData5_2, "source")  <- "5 person - Day 2"
simData5_3 <- read.csv("./DrawProfileTest_5p3_24hr67/TestToolOutput.csv")
attr(simData5_3, "source")  <- "5 person - Day 3"
simData5_4 <- read.csv("./DrawProfileTest_5p4_24hr67/TestToolOutput.csv")
attr(simData5_4, "source")  <- "5 person - Day 4"
simData5_5 <- read.csv("./DrawProfileTest_5p5_24hr67/TestToolOutput.csv")
attr(simData5_5, "source")  <- "5 person - Day 5"
simData5_6 <- read.csv("./DrawProfileTest_5p6_24hr67/TestToolOutput.csv")
attr(simData5_6, "source")  <- "5 person - Day 6"
simData5_7 <- read.csv("./DrawProfileTest_5p7_24hr67/TestToolOutput.csv")
attr(simData5_7, "source")  <- "5 person - Day 7"

simData_0.25 <- read.csv("./slowFlowTest_0.25/TestToolOutput.csv")
attr(simData_0.25, "source")  <- "Slow Flow Test 0.25 gpm"
simData_0.3 <- read.csv("./slowFlowTest_0.3/TestToolOutput.csv")
attr(simData_0.3, "source")  <- "Slow Flow Test 0.3 gpm"
simData_0.4 <- read.csv("./slowFlowTest_0.4/TestToolOutput.csv")
attr(simData_0.4, "source")  <- "Slow Flow Test 0.4 gpm"
simData_0.5 <- read.csv("./slowFlowTest_0.5/TestToolOutput.csv")
attr(simData_0.5, "source")  <- "Slow Flow Test 0.5 gpm"
simData_0.6 <- read.csv("./slowFlowTest_0.6/TestToolOutput.csv")
attr(simData_0.6, "source")  <- "Slow Flow Test 0.6 gpm"
simData_0.75 <- read.csv("./slowFlowTest_0.75/TestToolOutput.csv")
attr(simData_0.75, "source")  <- "Slow Flow Test 0.75 gpm"
simData_1.0 <- read.csv("./slowFlowTest_1.0/TestToolOutput.csv")
attr(simData_1.0, "source")  <- "Slow Flow Test 1.0 gpm"




# testlist <- list(simData4_1, simData4_2, simData4_3, simData4_4, simData4_5, simData4_6, simData4_7, simData5_1, simData5_2, simData5_3, simData5_4, simData5_5, simData5_6, simData5_7)
testlist <- list(simData_0.25, simData_0.3, simData_0.4, simData_0.5, simData_0.6, simData_0.75, simData_1.0)

lapply(testlist, function(simData){
  simData$inputPower <- (simData$input_kWh1 + simData$input_kWh2 + simData$input_kWh3) * 60000
  simData$outputPower <- (simData$output_kWh1 + simData$output_kWh2 + simData$output_kWh3) * 60000
  simData$aveTankTemp <- (simData$simTcouples1 + simData$simTcouples2 + simData$simTcouples3 + simData$simTcouples4 + simData$simTcouples5 + simData$simTcouples6)/6
  simData_m <- melt(simData, id = "minutes")
  
  
  varGuide <- data.frame("variable" = c("draw", "inputPower", "outputPower",
                                        "simTcouples1", "simTcouples2", "simTcouples3",
                                        "simTcouples4", "simTcouples5", "simTcouples6",
                                        "aveTankTemp", "Ta"),
                         "units" = c("Gallons", rep("Watts", 2),
                                     rep("Fahrenheit", 8)),
                         "category" = c("Draw", "Input Power", "Output Power",
                                        rep("Thermocouples", 6), "Average Tank Temp",
                                        "Ambient Temp"))
  
  simData_m <- merge(simData_m, varGuide)
  
  ggplot(data = simData_m, aes(x = minutes, y = value, group = variable, col = variable)) + theme_bw() + 
    geom_line() + facet_grid(units~., scales = "free_y") +
    ggtitle(attr(simData, "source"))

  ggsave(filename = paste0(attr(simData, "source"), ".png"))
  })

