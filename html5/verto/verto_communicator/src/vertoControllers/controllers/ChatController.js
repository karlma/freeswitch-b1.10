(function() {
  'use strict';

  angular
  .module('vertoControllers')
  .controller('ChatController', ['$scope', '$rootScope', '$http',
    '$location', '$anchorScroll', '$timeout', 'verto',
    function($scope, $rootScope, $http, $location, $anchorScroll, $timeout,
      verto) {
      console.debug('Executing ChatController.');

      function scrollToChatBottom() {
        // Going to the bottom of chat messages.
        var obj = document.querySelector('.chat-messages');
        obj.scrollTop = obj.scrollHeight;
        //var chat_messages_top = jQuery('.chat-messages').scrollTop();
        //var marker_position = jQuery('#chat-message-bottom').position().top;
        //jQuery('.chat-messages').scrollTop(chat_messages_top + marker_position);
      }

      var CLEAN_MESSAGE = '';

      function clearConferenceChat() {
        $scope.members = [];
        $scope.messages = [];
        $scope.message = CLEAN_MESSAGE;
      }
      clearConferenceChat();

      $scope.$watch('activePane', function() {
        if ($scope.activePane == 'chat') {
          $rootScope.chat_counter = 0;
        }
        $rootScope.activePane = $scope.activePane;
      });

      $rootScope.$on('chat.newMessage', function(event, data) {
        data.created_at = new Date();
        console.log('chat.newMessage', data);
        $scope.$apply(function() {
          $scope.messages.push(data);
          if (data.from != verto.data.name && (!$scope.chatStatus ||
              $scope.activePane != 'chat')) {
            ++$rootScope.chat_counter;
          }
          $timeout(function() {
            scrollToChatBottom();
          }, 300);
        });
      });

      function findMemberByUUID(uuid) {
        var found = false;
        for (var idx in $scope.members) {
          var member = $scope.members[idx];
          if (member.uuid == uuid) {
            found = true;
            break;
          }
        }
        if (found) {
          return idx;
        } else {
          return -1;
        }
      }

      function translateMember(member) {
        return {
          'uuid': member[0],
          'id': member[1][0],
          'number': member[1][1],
          'name': member[1][2],
          'codec': member[1][3],
          'status': JSON.parse(member[1][4]),
          'email': member[1][5].email
        };
      }

      function addMember(member) {
        $scope.members.push(translateMember(member));
      }

      $rootScope.$on('members.boot', function(event, members) {
        console.log('members.boot', event, members);
        $scope.$apply(function() {
          clearConferenceChat();
          for (var idx in members) {
            var member = members[idx];
            addMember(member);
            console.log($scope.members);
          }
        })
      });

      $rootScope.$on('members.add', function(event, member) {
        $scope.$apply(function() {
          addMember(member);
        });
      });

      $rootScope.$on('members.del', function(event, uuid) {
        $scope.$apply(function() {
          var memberIdx = findMemberByUUID(uuid);
          if (memberIdx != -1) {
            // Removing the member.
            $scope.members.splice(memberIdx, 1);
          }
        });
      });

      $rootScope.$on('members.update', function(event, member) {
        member = translateMember(member);
        var memberIdx = findMemberByUUID(member.uuid);
        if (memberIdx < 0) {
          console.log('Didn\'t find the member uuid ' + member.uuid);
        } else {
          $scope.$apply(function() {
            console.log('Updating', memberIdx, ' <', $scope.members[memberIdx],
              '> with <', member, '>');
            angular.extend($scope.members[memberIdx], member);
          });
        }
      });

      $rootScope.$on('members.clear', function(event) {
        $scope.$apply(function() {
          clearConferenceChat();
          $scope.closeChat();
        });
      });

      /**
       * Public methods.
       */
      $scope.send = function() {
        verto.sendMessage($scope.message, function() {
          $scope.message = CLEAN_MESSAGE;
        });
      };

      // Participants moderation.
      $scope.confKick = function(memberID) {
        console.log('$scope.confKick');
        verto.data.conf.kick(memberID);
      };

      $scope.confMuteMic = function(memberID) {
        console.log('$scope.confMuteMic');
        verto.data.conf.muteMic(memberID);
      };

      $scope.confMuteVideo = function(memberID) {
        console.log('$scope.confMuteVideo');
        verto.data.conf.muteVideo(memberID);
      };

      $scope.confPresenter = function(memberID) {
        console.log('$scope.confPresenter');
        verto.data.conf.presenter(memberID);
      };

      $scope.confVideoFloor = function(memberID) {
        console.log('$scope.confVideoFloor');
        verto.data.conf.videoFloor(memberID);
      };

      $scope.confBanner = function(memberID) {
        console.log('$scope.confBanner');
        var text = 'New Banner';
        verto.data.conf.banner(memberID, text);
      };

      $scope.confVolumeDown = function(memberID) {
        console.log('$scope.confVolumeDown');
        verto.data.conf.volumeDown(memberID);
      };

      $scope.confVolumeUp = function(memberID) {
        console.log('$scope.confVolumeUp');
        verto.data.conf.volumeUp(memberID);
      };

      $scope.confTransfer = function(memberID) {
        console.log('$scope.confTransfer');
        var exten = '1800';
        verto.data.conf.transfer(memberID, exten);
      };
    }
  ]);

})();