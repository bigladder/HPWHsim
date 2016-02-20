# Chip test...

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")
library(EcotopePackage)

c("minutes", "test", "model", "type")
chipFiles <- dir("fromChip/", full.names = TRUE, pattern = "csv$")

allChipResults <- do.call('rbind', lapply(chipFiles, function(chipFile) {
  abc <- read.csv(chipFile, skip = 2)
  names(abc)[grep("draw.*gal", names(abc))] <- "flow"
  inputVars <- grep("src.*In", names(abc))
  outputVars <- grep("src.*Out", names(abc))
  if(length(inputVars) > 1) {
    abc$inputPower <- apply(abc[, inputVars], 1, sum) * 60
    abc$outputPower <- apply(abc[, outputVars], 1, sum) * 60
  } else {
    abc$inputPower <- abc[, inputVars] * 1000 * 60
    abc$outputPower <- abc[, outputVars] * 1000 * 60
  }
  names(abc) <- gsub("tcouple([0-9]+).+", "tcouples\\1", names(abc))
  tcoupleVars <- grep("tcouples", names(abc))
  abc[, tcoupleVars] <- abc[, tcoupleVars] * 1.8 + 32
  abc$aveTankTemp <- apply(abc[, tcoupleVars], 1, mean)
  names(abc)[names(abc) == "tEnv..C."] <- "Ta"
  abc$minutes <- abc$minDay + (abc$day - 1) * 1440
  abc$model <- "Voltex60"
  abc$test <- "Daily_1"
  abc$type <- "CSE"
  # abc <- abc[abc$inputPower >= 0, ]
  
  
  abc <- abc[, c("minutes", "test", "model", "type",
                 "flow", "inputPower", "outputPower",
                 "tcouples1", "tcouples2", "tcouples3",
                 "tcouples4", "tcouples5", "tcouples6",
                 "aveTankTemp", "Ta")]
  abc
}))


write.csv(file = "HpwhTestTool/allChipResults.csv", allChipResults, row.names = FALSE)




