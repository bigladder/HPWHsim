library(reshape2)
library(ggplot2)

simData1_1 <- read.csv("./DrawProfileTest_1p1_24hr67/TestToolOutput.csv")
attr(simData1_1, "source")  <- "1 person - Day 1"
simData1_2 <- read.csv("./DrawProfileTest_1p2_24hr67/TestToolOutput.csv")
attr(simData1_2, "source")  <- "1 person - Day 2"
simData1_3 <- read.csv("./DrawProfileTest_1p3_24hr67/TestToolOutput.csv")
attr(simData1_3, "source")  <- "1 person - Day 3"
simData1_4 <- read.csv("./DrawProfileTest_1p4_24hr67/TestToolOutput.csv")
attr(simData1_4, "source")  <- "1 person - Day 4"
simData1_5 <- read.csv("./DrawProfileTest_1p5_24hr67/TestToolOutput.csv")
attr(simData1_5, "source")  <- "1 person - Day 5"
simData1_6 <- read.csv("./DrawProfileTest_1p6_24hr67/TestToolOutput.csv")
attr(simData1_6, "source")  <- "1 person - Day 6"
simData1_7 <- read.csv("./DrawProfileTest_1p7_24hr67/TestToolOutput.csv")
attr(simData1_7, "source")  <- "1 person - Day 7"

simData2_1 <- read.csv("./DrawProfileTest_2p1_24hr67/TestToolOutput.csv")
attr(simData2_1, "source")  <- "2 person - Day 1"
simData2_2 <- read.csv("./DrawProfileTest_2p2_24hr67/TestToolOutput.csv")
attr(simData2_2, "source")  <- "2 person - Day 2"
simData2_3 <- read.csv("./DrawProfileTest_2p3_24hr67/TestToolOutput.csv")
attr(simData2_3, "source")  <- "2 person - Day 3"
simData2_4 <- read.csv("./DrawProfileTest_2p4_24hr67/TestToolOutput.csv")
attr(simData2_4, "source")  <- "2 person - Day 4"
simData2_5 <- read.csv("./DrawProfileTest_2p5_24hr67/TestToolOutput.csv")
attr(simData2_5, "source")  <- "2 person - Day 5"
simData2_6 <- read.csv("./DrawProfileTest_2p6_24hr67/TestToolOutput.csv")
attr(simData2_6, "source")  <- "2 person - Day 6"
simData2_7 <- read.csv("./DrawProfileTest_2p7_24hr67/TestToolOutput.csv")
attr(simData2_7, "source")  <- "2 person - Day 7"

simData3_1 <- read.csv("./DrawProfileTest_3p1_24hr67/TestToolOutput.csv")
attr(simData3_1, "source")  <- "3 person - Day 1"
simData3_2 <- read.csv("./DrawProfileTest_3p2_24hr67/TestToolOutput.csv")
attr(simData3_2, "source")  <- "3 person - Day 2"
simData3_3 <- read.csv("./DrawProfileTest_3p3_24hr67/TestToolOutput.csv")
attr(simData3_3, "source")  <- "3 person - Day 3"
simData3_4 <- read.csv("./DrawProfileTest_3p4_24hr67/TestToolOutput.csv")
attr(simData3_4, "source")  <- "3 person - Day 4"
simData3_5 <- read.csv("./DrawProfileTest_3p5_24hr67/TestToolOutput.csv")
attr(simData3_5, "source")  <- "3 person - Day 5"
simData3_6 <- read.csv("./DrawProfileTest_3p6_24hr67/TestToolOutput.csv")
attr(simData3_6, "source")  <- "3 person - Day 6"
simData3_7 <- read.csv("./DrawProfileTest_3p7_24hr67/TestToolOutput.csv")
attr(simData3_7, "source")  <- "3 person - Day 7"

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
simData_0.5 <- read.csv("./slowFlowTest_0.5/TestToolOutput.csv")
attr(simData_0.5, "source")  <- "Slow Flow Test 0.5 gpm"
simData_0.75 <- read.csv("./slowFlowTest_0.75/TestToolOutput.csv")
attr(simData_0.75, "source")  <- "Slow Flow Test 0.75 gpm"
simData_1.0 <- read.csv("./slowFlowTest_1.0/TestToolOutput.csv")
attr(simData_1.0, "source")  <- "Slow Flow Test 1.0 gpm"




testlist <- list(simData1_1, simData1_2, simData1_3, simData1_4, simData1_5, simData1_6, simData1_7,
                 simData2_1, simData2_2, simData2_3, simData2_4, simData2_5, simData2_6, simData2_7,
                 simData3_1, simData3_2, simData3_3, simData3_4, simData3_5, simData3_6, simData3_7,
                 simData4_1, simData4_2, simData4_3, simData4_4, simData4_5, simData4_6, simData4_7,
                 simData5_1, simData5_2, simData5_3, simData5_4, simData5_5, simData5_6, simData5_7)
# testlist <- list(simData_0.25, simData_0.5, simData_0.75, simData_1.0)

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

