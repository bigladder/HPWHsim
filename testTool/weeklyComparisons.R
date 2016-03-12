# Weekly tests...
library(ggplot2)
library(plyr)
# Exercise the weekly draw profiles...


# Copy the weekly draw profiles???
tests <- c("Weekly_1", "Weekly_2", "Weekly_3", "Weekly_4", "Weekly_5")
models <- makes <- c("AOSmith60", "AOSmith80",
                     "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
                     "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
                     "SandenGAU", "SandenGES", "Stiebel220e",
                     "Generic1", "Generic2", "Generic3")
copyWeeklyData <- function(Ta, inletT, setpoint = 127) {
  lapply(models, function(model) {
    lapply(tests, function(test) {
      dirToRef <- paste("tests", test, sep = "/")
      dirToWork <- paste("models", model, test, sep = "/")
      dir.create(dirToWork)
      file.copy(paste(dirToRef, "drawschedule.csv", sep = "/"), 
                paste(dirToWork, "drawschedule.csv", sep = "/"))
      
      # Write out the ambient schedule. Do I want to add
      ambientScheduleFile <- paste(dirToWork, "ambientTschedule.csv", sep = "/")
      writeLines(text = paste("default", (Ta - 32) / 1.8), con = ambientScheduleFile)
      
      # Write out the evaporator schedule
      evaporatorScheduleFile <- paste(dirToWork, "evaporatorTschedule.csv", sep = "/")
      writeLines(text = paste("default", (Ta - 32) / 1.8), con = evaporatorScheduleFile)
      
      # Write out the inlet schedule
      inletScheduleFile <- paste(dirToWork, "inletTschedule.csv", sep = "/")
      writeLines(text = paste("default", (inletT - 32) / 1.8), con = inletScheduleFile)
      
      # Write out the DR schedule
      DRScheduleFile <- paste(dirToWork, "DRschedule.csv", sep = "/")
      writeLines(text = "default 1\nminutes,OnOff", con = DRScheduleFile)
      
      # What was the setpoint???
      testInfoFile <- paste(dirToWork, "testInfo.txt", sep = "/")
      writeLines(text = paste0("length_of_test ", 1440*7,
                               "\nsetpoint ", (setpoint - 32) / 1.8),
                 con = testInfoFile)    
    })
  })
}

runSimsWeekly <- function() {
  
  # Run the weekly stuff...
  # Assuming that the Weekly draw profiles have been copied... see generics.R

  # Simulate every combination of test and model
  allResults <- do.call('rbind', lapply(models, function(model) {
    # Loop over tests...
    do.call('rbind', lapply(tests, function(test) {
      print(paste("Simulating model", model, "test", test))
      # Run the Simulation
      system(paste("./testTool.x", test, model))
      
      # Read the results
      dset <- try(read.csv(paste("models/", model, "/", test, "/", "TestToolOutput.csv", sep = "")))
      if(inherits(dset, "try-error")) {
        print(paste("No Output for", model, test)) 
        return(NULL)
      }
      dset$test <- test
      dset$model <- model
      
      # Parse the energy outputs
      inputVars <- grep("input_kWh", names(dset))
      if(length(inputVars) > 1) {
        dset$inputTotal_W <- apply(dset[, inputVars], 1, sum) * 60000
      } else {
        dset$inputTotal_W <- dset[, inputVars] * 60000
      }
      dset <- dset[, -inputVars]
      
      outputVars <- grep("output_kWh", names(dset))
      if(length(outputVars) > 1) {
        dset$outputTotal_W <- apply(dset[, outputVars], 1, sum) * 60000
      } else {
        dset$outputTotal_W <- dset[, outputVars] * 60000
      }
      dset <- dset[, -outputVars]
      # print(head(dset))
      dset
    }))
  }))
  
  # Convert all temperature variables from C to F
  tempVars <- grep("Tcouple", names(allResults))
  allResults[, tempVars] <- allResults[, tempVars] * 1.8 + 32
  allResults$aveTankTemp <- apply(allResults[, tempVars], 1, mean)
  
  
  allResults[] <- lapply(names(allResults), function(v) {
    if(length(grep("input_kWh", v)) | length(grep("output_kWh", v))) {
      NULL
    } else {
      allResults[, v]
    }
  })
  
  
  names(allResults)[names(allResults) == "draw"] <- "flow"
  names(allResults)[names(allResults) == "simTcouples1"] <- "tcouples1"
  names(allResults)[names(allResults) == "simTcouples2"] <- "tcouples2"
  names(allResults)[names(allResults) == "simTcouples3"] <- "tcouples3"
  names(allResults)[names(allResults) == "simTcouples4"] <- "tcouples4"
  names(allResults)[names(allResults) == "simTcouples5"] <- "tcouples5"
  names(allResults)[names(allResults) == "simTcouples6"] <- "tcouples6"
  allResults$inputPower <- allResults$inputTotal_W
  allResults$outputPower <- allResults$outputTotal_W
  
  allResults <- allResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                               "tcouples1", "tcouples2", "tcouples3",
                               "tcouples4", "tcouples5", "tcouples6",
                               "aveTankTemp", "Ta")]
  allResults$type <- "Simulated"
  
  allResults
}

calcEf <- function(x, inletT) {
  if(!nrow(x)) return(NA)
  tankSize <- x$size[1]
  input_btu <- sum(x$inputPower / 60 * 3.412, na.rm = TRUE)
  delivered_btu <- sum(x$flow * 8.34 * (x$tcouples6[1] - inletT))
  delta_btu <- (x$aveTankTemp[1] - x$aveTankTemp[nrow(x)]) * tankSize * 8.34
  (delivered_btu - delta_btu) / input_btu
}


collapseWeekly <- function(allResults, inletT) {
  
  sizes <- data.frame("model" = sort(unique(allResults$model)),
                      "size" = c(57, 75, 42, 64, 75, 45, 45, 45, 45, 45, 45, 45, 83, 40, 56))
  
  # allResults2 <- allResults
  allResults <- merge(allResults, sizes)
  wsum <- ddply(allResults, .(model, test), function(x) {
    calcEf(x, inletT)
  })
  names(wsum) <- c("model", "test", "SEF")
  wsum
  
}


Tas <- seq(from = 30, to = 90, by = 2)
inlets <- seq(from = 45, to = 70, by = 5)
# Tas <- c(50, 67)
inlets <- c(50)

results <- do.call('rbind', lapply(Tas, function(Ta) {
  do.call('rbind', lapply(inlets, function(inletT) {
    # Copy Lab Data
    copyWeeklyData(Ta, inletT)
    
    # Run Sims
    allResults <- runSimsWeekly()
    
    # Read and collapse
    wsum <- collapseWeekly(allResults, inletT)
    wsum$Ta <- Ta
    wsum$inletT <- inletT
    
    wsum
  }))
}))

results$tainlet <- paste("Ta", results$Ta, "inlet", results$inletT, sep = ".")

save(file = "results.rda", results)


load("results.rda")

allMeans <- aggregate(SEF ~ model, data = results, FUN = mean)
allMeans <- arrange(allMeans, SEF)
wsum2 <- aggregate(SEF ~ model + inletT + Ta + tainlet, data = results, FUN = mean)

# Make the weighted average and plot by temp???
fracs <- c(.2, .46, .12, .12, .1)
df <- data.frame("test" = c("Weekly_1", "Weekly_2", "Weekly_3", "Weekly_4", "Weekly_5"),
                 "frac" = fracs)
results2 <- ddply(results, .(model, Ta), function(x){
  x <- merge(x, df)
  c("SEF" = sum(x$SEF * x$frac))
})

results$model <- factor(results$model, levels = allMeans$model)
temps <- c(40, 50, 60, 70)
wplot <- ggplot(results[results$Ta %in% temps, ]) + theme_bw() + 
  geom_point(aes(x = SEF, y = model, col = test)) +
  # geom_point(data = wsum2, aes(x = SEF, y = model), shape = 10) +
  geom_point(data = results2[results2$Ta %in% temps, ], 
             aes(SEF, model), shape = 10) +
  ggtitle("Weekly SEEM Draw Profiles\nAll Simulated Results") +
  xlab("Simple Energy Factor") + ylab("Model") +
  scale_colour_discrete(name = "Test") +
  facet_wrap(~Ta)
wplot
ggsave(file = "graphs/wplot.png", wplot, width = 7, height = 5)

results <- results[results$SEF < 1000, ]
ggplot(results) + theme_bw() + 
  geom_line(aes(Ta, SEF, col = test)) +
  facet_wrap(~model)


ge2014Plot <- ggplot(results[grep("GE50", results$model), ]) + theme_bw() + 
  geom_line(aes(x = Ta, y = SEF, col = test, linetype = model)) +
  ggtitle("GE2014 Why You Make No Sense") +
  xlab("Air Temperature (F)") +
  ylab("Simple Energy Factor")
ge2014Plot
ggsave(file = "graphs/ge2014Plot.png", ge2014Plot, width = 7, height = 5)

# Let's look at the HPTUs...

hptuPlot <- ggplot(results[grep("HPTU", results$model), ]) + theme_bw() + 
  geom_line(aes(x = Ta, y = SEF, col = model, linetype = model)) +
  ggtitle("AOSmith HPTU Weekly Draw Profile Results") +
  xlab("Air Temperature (F)") +
  ylab("Simple Energy Factor") +
  facet_wrap(~test)
hptuPlot
ggsave(file = "graphs/hptuPlot.png", hptuPlot, width = 7, height = 5)



allMeans <- aggregate(SEF ~ model, data = results, FUN = mean)
allMeans <- arrange(allMeans, SEF)

results2$model <- factor(results2$model, levels = allMeans$model)
modelsToUse <- c("GE502014", "AOSmithHPTU66", "SandenGAU", "AOSmith60", "GEred", "AOSmithHPTU50", "AOSmithHPTU80")
sefByTemp <- ggplot(results2[results2$model %in% modelsToUse, ]) + theme_bw() + 
  geom_point(aes(Ta, SEF, col = model, shape = model)) +
  ggtitle("Weighted Weekly Draw Profile Simple Energy Factors") +
  xlab("Ambient Temp (F)") + ylab("Simple Energy Factor") +
  scale_x_continuous(breaks = seq(30, 90, by = 5)) +
  scale_colour_discrete(name = "Model") +
  scale_shape_manual(name = "Model", values = 2:9)
sefByTemp
ggsave(file = "graphs/sefByTemp.png", sefByTemp, width = 7, height = 5)

# Pick three temps... 50, 60, and 70


ggplot(results2[results2$Ta %in% c(50, 60, 70), ]) + theme_bw() + 
  geom_point(aes(x = SEF, y = model, col = factor(Ta)))


ggplot(results2[results2$Ta >= 50 & results2$Ta <= 80, ]) + theme_bw() + 
  geom_point(aes(x = SEF, y = model, col = Ta)) +
  scale_colour_continuous(low = "black", high = "green")



