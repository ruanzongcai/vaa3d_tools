%  Figure generation for smartscope 2 data
% based on scanAnalysisScripts, this script will be updated as changes are
% made to output figures, but figure generation should be accomplished
% simply by running this script. 
%   Sun April 10 2016

%  reviewing data again 4/11 further reduced the number of legitimate scans
%  (e.g. not crazy overtracing due to low threshold or mislocated starting
%  tiles due to xy stage movement after preview was taken.)


%Data analysis for smartscope 2 
% starting 2016.03.28

% 1. Data Entry



%   only a subset of data is worth analyzing for s2 performance.  
%  these data will be copied into a special directory, /local2/s2Data/  and
%  organized into directories for each cell imaged.

%  the post-PV files are stored on my workstation, including .xml files transfered
%  from the rig machine.
batchTopDirectory = '/local2/s2Data'
cd(fullfile(batchTopDirectory))

batch = dir(fullfile(batchTopDirectory,'cell*'))
tic


for i = 1:numel(batch)


   celliDir = dir(fullfile(batchTopDirectory, batch(i).name));

    celliDir = celliDir([celliDir(:).isdir]') ; % just get scan directories
    keepList = true(1,numel(celliDir));
   for j = 1:numel(celliDir)  %  all scans, even aborted ones
       if (sum(findstr(celliDir(j).name, '2016'))==0)  % not a normal scan folder
           keepList(j) = false;
       end
   end
   celliDir = celliDir(keepList);
    for j = 1:numel(celliDir)  %  all scans, even aborted ones    
        scanjDir = dir(fullfile(batchTopDirectory, batch(i).name, celliDir(j).name,'*.xml'));
        if numel(scanjDir)>0
            cellData{i}(j) = scanDataFromXMLDir(fullfile(batchTopDirectory, batch(i).name, celliDir(j).name));
            
        end
        
   end
   

end
toc
% 914s for 25 cells
normalScans = numel(cellData)
%%  now add the adaptive and grid scans for comparison



adaptiveScans = dir(fullfile(batchTopDirectory, 'cell001','adaptive'));
for i = 1:numel(adaptiveScans)
    if findstr(adaptiveScans(i).name, '.')
                adaptiveScans(i).isdir = false;
        continue
        else
    end
adaptiveScans(i).fullDirPath = fullfile(batchTopDirectory, 'cell001','adaptive',adaptiveScans(i).name)
end


adaptiveScans2 = dir(fullfile(batchTopDirectory, 'cell002','adaptive'));
for i = 1:numel(adaptiveScans2)
    if findstr(adaptiveScans2(i).name, '.')
                adaptiveScans2(i).isdir = false;
        continue
    end
adaptiveScans2(i).fullDirPath = fullfile(batchTopDirectory, 'cell002','adaptive',adaptiveScans2(i).name)
end
adaptiveScans = [adaptiveScans;adaptiveScans2]




gridScans = dir(fullfile(batchTopDirectory, 'cell001', 'grid'));
for i = 1:numel(gridScans)
    if findstr(gridScans(i).name, '.')
        gridScans(i).isdir = false;
        continue
        
    end
gridScans(i).fullDirPath = fullfile(batchTopDirectory, 'cell001','grid',gridScans(i).name)
end


otherScans = [adaptiveScans; gridScans]
otherScans = otherScans([otherScans(:).isdir]')
%%  

for i = 1:numel(otherScans)


            cellData{1+normalScans}(i) = scanDataFromXMLDir(fullfile(otherScans(i).fullDirPath));
       
  
   

end



%% 2.  Analysis

% determine difference between summed tile volume and the actual scanned
% volume [estimate from (whole micron?) binary images] 



% for a set of tile locations, build a binary array to model the scan area:

for i = 1:numel(cellData)
    for j = 1:numel(cellData{i})
        
        tileSetij = cellData{i}(j).tileLocations;
        nTiles = numel(tileSetij)
        cellData{i}(j).ignore = false;
        
        
        if ( nTiles<3 )|| isempty(cellData{i}(j).folderName) || sum(strfind(cellData{i}(j).folderName, '.'))>0
            cellData{i}(j).ignore = true;
            continue
        else
            ijdir = dir(cellData{i}(j).folderName)
            isGrid = false;
            for s = 1:numel(ijdir)
                isGrid = isGrid || (sum(strfind( ijdir(s).name, 'Grid'))>0);
            end
            cellData{i}(j).isGridScan = isGrid;
            
            if cellData{i}(j).isGridScan
                'gridscan'
            end
            allLocations = cell2mat(tileSetij');
            [bigRectx, bigRecty] = meshgrid(floor(min(allLocations(:,1))):ceil(max(allLocations(:,3))),floor(min(allLocations(:,2))):ceil(max(allLocations(:,4))));
            bigRect = false(size(bigRectx));
            for k = 1:nTiles
                bigRect(:) = bigRect(:)| ((bigRectx(:)>allLocations(k,1)) &( bigRectx(:)<=allLocations(k,3) )& (bigRecty(:)>allLocations(k,2)) &( bigRecty(:)<=allLocations(k,4) ));
                
                
            end
            cd(fullfile(cellData{i}(j).folderName,'..'))
            dString = pwd;
            
            nnTry = str2double(dString(end-2:end))+1
            if isnan(nnTry)
                nnTry = 10+(j<=8)
            end
            cellData{i}(j).neuronNumber = nnTry;
 
            
            cellData{i}(j).boundingBoxArea = numel(bigRect);
            cellData{i}(j).imagedArea = sum(bigRect(:));
            cellData{i}(j).tileAreas = (allLocations(:,4)-allLocations(:,2)).*(allLocations(:,3)-allLocations(:,1));
            cellData{i}(j).totalTileArea = sum(cellData{i}(j).tileAreas);
            cellData{i}(j).extraScanning = cellData{i}(j).totalTileArea-cellData{i}(j).imagedArea;
            cellData{i}(j).boundingBoxSparsity = cellData{i}(j).totalTileArea/numel(bigRect);
            cellData{i}(j).lagTimes =  diff(cellData{i}(j).tileStartTimes)-cellData{i}(j).allTileTimes(1:end-1);
            cellData{i}(j).totalTime = cellData{i}(j).tileStartTimes(end)-cellData{i}(j).tileStartTimes(1)+cellData{i}(j).allTileTimes(end)+min(cellData{i}(j).lagTimes);
            cellData{i}(j).minTotalTime = sum(cellData{i}(j).allTileTimes(:)+min(cellData{i}(j).lagTimes));
            cellData{i}(j).minImagingOnly = sum(cellData{i}(j).allTileTimes(:));
            cellData{i}(j).estimatedMinLag = min( cellData{i}(j).lagTimes);
            cellData{i}(j).estimatedTimePerTileArea = mean((cellData{i}(j).allTileTimes(:)+cellData{i}(j).estimatedMinLag)./cellData{i}(j).tileAreas);
            cellData{i}(j).estimatedGridTime = numel(bigRect)*cellData{i}(j).estimatedTimePerTileArea;
        end
    end
end

cd(fullfile(batchTopDirectory))

% extract lag times = (difference between sequential tiles) - tiletime.
% this will include convert/load times but should also show initial big lag
% followed by minimal lags in continuous imaging mode. tough to extract necessary from unnecessary
% delays, though.



% extract 'extra' time (difference between tiletime*N and total time)

%  total time vs tile size for each neuron (N  = 3)

%  total volume vs tile size for each neuron (N = 3)
%% plotting
figure
neuronNumbers = []
neuronData={}
timeSummary = {}
for i = 1:numel(cellData)
    for j = 1:numel(cellData{i})
                          if isfield(cellData{i}(j),'neuronNumber')    
                nn  = cellData{i}(j).neuronNumber;
neuronNumbers = [neuronNumbers; nn];

                          end
    end
end


a = unique(neuronNumbers)
for ii = 1:numel(a)
    neuronData{a(ii)}=[0,0,0,0,0,0,0,0]
                timeSummary{a(ii)} = [0,0,0]
end

                myCmap = colormap(jet(numel(a)+1));
for i = 1:numel(cellData)
    for j = 1:numel(cellData{i})
        
        tileSetij = cellData{i}(j).tileLocations;
        nTiles = numel(tileSetij);
        if cellData{i}(j).ignore
            continue
        else
            if isfield(cellData{i}(j),'neuronNumber')    
                nn  = cellData{i}(j).neuronNumber;

                    neuronData{nn} = [neuronData{nn} ; [mean(sqrt(cellData{i}(j).tileAreas)),cellData{i}(j).imagedArea,mean(sqrt(cellData{i}(j).tileAreas)),cellData{i}(j).totalTime, cellData{i}(j).totalTileArea, cellData{i}(j).boundingBoxArea,i,j]]

                    timeSummary{nn}= [timeSummary{nn}; [cellData{i}(j).estimatedGridTime,cellData{i}(j).minTotalTime , cellData{i}(j).totalTime]/( mean((cellData{i}(j).allTileTimes(:)+cellData{i}(j).estimatedMinLag)))];
           

%                     subplot(2,1,1), hold all, plot(mean(sqrt(cellData{i}(j).tileAreas)),cellData{i}(j).imagedArea,'o-', 'DisplayName', cellData{i}(j).folderName, 'color', myCmap(nn,:))
%                     subplot(2,1,2), hold all, plot(mean(sqrt(cellData{i}(j).tileAreas)),cellData{i}(j).totalTime, '*-', 'DisplayName', cellData{i}(j).folderName, 'color', myCmap(nn,:))
%                     plot(mean(sqrt(cellData{i}(j).tileAreas)),cellData{i}(j).minTotalTime, '*-','color', myCmap(nn,:))
                end 
            
        end
    end
end
%
a = unique(neuronNumbers)
for ii = 1:numel(a)
neuronData{a(ii)} = neuronData{a(ii)}(neuronData{a(ii)}(:,1)~=0,:)
[ neuronData{a(ii)}, rs] = sortrows(neuronData{a(ii)},1)
timeSummary{a(ii)} = timeSummary{a(ii)}(timeSummary{a(ii)}(:,1)~=0,:);
timeSummary{a(ii)} = timeSummary{a(ii)}(rs,:)
% if there are multiple scans at the same tile size, I'll plot the mean and
% min-max as errorbars.
xVals = unique(neuronData{a(ii)}(:,1))
xToPlot = xVals
y1ToPlot = zeros(size(xToPlot))
yErrorU1 = y1ToPlot
yErrorL1 = yErrorU1
yErrorU2= yErrorU1
yErrorL2= yErrorU2
y2ToPlot = yErrorU1
for jj = 1:numel(xVals)
    y1ToPlot(jj)   = mean(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),2))
    yMax1 = max(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),2));
    yMin1 = min(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),2));
    yErrorU1(jj) = yMax1-y1ToPlot(jj)
    yErrorL1(jj) = y1ToPlot(jj)-yMin1
    
    y2ToPlot(jj)   = mean(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),4))
    yMax2 = max(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),4));
    yMin2 = min(neuronData{a(ii)}(neuronData{a(ii)}(:,1)==xVals(jj),4));
    yErrorU2(jj) = yMax2-y2ToPlot(jj)
    yErrorL2(jj) = y2ToPlot(jj)-yMin2 
    
end

subplot(2,2,1),  hold all,   plot(neuronData{a(ii)}(:,1),neuronData{a(ii)}(:,2), '*','color', myCmap(ii,:),'DisplayName', cellData{neuronData{a(ii)}(end,7)}(neuronData{a(ii)}(end,8)).folderName);
errorbar(xToPlot,y1ToPlot, yErrorL1, yErrorU1,'color', myCmap(ii,:))
bip
subplot(2,2,2),    hold all, plot(neuronData{a(ii)}(:,3),neuronData{a(ii)}(:,4), '*','color', myCmap(ii,:),'DisplayName',cellData{neuronData{a(ii)}(end,7)}(neuronData{a(ii)}(end,8)).folderName);
errorbar(xToPlot,y2ToPlot, yErrorL2, yErrorU2,'color', myCmap(ii,:))
bip

subplot(2,2,3) ,   hold all, plot(neuronData{a(ii)}(:,1), timeSummary{a(ii)}(:,3)./timeSummary{a(ii)}(:,1),'color', myCmap(ii,:))
 bip
subplot(2,2,4), hold all, plot(neuronData{a(ii)}(:,1), timeSummary{a(ii)}(:,3)./timeSummary{a(ii)}(:,2)-1,'color', myCmap(ii,:))
bip
end
subplot(2,2,3) ,  plot([0 500], [1 1],'k')
%subplot(2,2,3) ,   hold all, plot(neuronData{a(ii)}(:,5),neuronData{a(ii)}(:,6), '*-','color', myCmap(ii,:),'DisplayName',cellData{neuronData{a(ii)}(end,7)}(neuronData{a(ii)}(end,8)).folderName);
%subplot(2,2,4) ,   hold all, plot(neuronData{a(ii)}(:,2),neuronData{a(ii)}(:,6), '*-','color', myCmap(ii,:),'DisplayName',cellData{neuronData{a(ii)}(end,7)}(neuronData{a(ii)}(end,8)).folderName);

%%  notes and comments.

%  cell000 neuronNumber = 1
%  2016_03_27_Sun_09_45  background was too high. but the tile size is the
%  same as 9_46.  for some reason some preview files got put in the data
%  directory, causing an overestimation of the tile size (which is taken
%  from the average of the tile sizes generated from the xml files)  9_45
%  and the problem tiles in 9_46 were removed to ./other/


% cell001 neuronNumber = 8

% cell002 neuronNumber = 9





% FORMER NAMES! stored in /data
% cell001  neuronNumber = 3 removed looks like crap. MOST tracing, discontinuous tile boundaries,
% poor labeling.

% cell002 (neuronNumber = 4)  THESE ARE ALL TEST SCANS OF NOISE




%%   notes and comments



%  A. /local2/2016_03_27_Sun_09_00/2016_03_27_Sun_10_18  ran away and kept
%  imaging forever.  initial threshold was 10,  189 slices 
% B. /local2/2016_03_27_Sun_09_00/2016_03_27_Sun_10_50   threshold was 15,  169 slices 
%  C. /local2/2016_03_27_Sun_09_00/2016_03_27_Sun_10_55   threshold was 10.  169 slices 
% imaging power was the same for all, but clear drop in fluorescence from
% A. to B.

%10_11 power 100
%10_14 power 110
%10_18-10_55 power 130

%  I loaded the center tile of all 5 scans of this neuron into FIJI and
%  generated the mean value of each slice. those data were loaded into
%  matlab as matrices 

% % plot the peak values of each:
% 
% peakData = [max(mean4022(:,2)), max(mean4026(:,2)),max(mean4036(:,2)),max(mean4159(:,2)),max(mean4170(:,2))]
% 
% backgroundData = [max(mean4022(1:50,2)), max(mean4026(1:50,2)),max(mean4036(1:50,2)),max(mean4159(1:50,2)),max(mean4170(1:50,2))]
% 
% plot(peakData), hold all, plot(backgroundData)
% 
% 
% %  very clear from looking at the data again that tile-to-tile stitching is
%  incorrect. specifically, it looks like the left and right edges of the
%  tiles are not correctly located in space.  we think we have 10-15%
%  overlap, but the image data is NOT actually overlapped.  since the field
%  size is taken directly from PV, it's hard to see how this can be
%  happening.  either the field size is wrong or the distance metric varies
%  across the field along the fastscan axis.  

%