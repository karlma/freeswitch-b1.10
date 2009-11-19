#!/bin/sh
#
# broadvoice - a library for the BroadVoice 16 and 32 codecs
#
# regression_tests.sh
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id: regression_tests.sh.in,v 1.1.1.1 2009/11/19 12:10:48 steveu Exp $
#

STDOUT_DEST=xyzzy
STDERR_DEST=xyzzy2
VECTOR_CLASS=@G722_1_VECTORS_FOR_TESTS@
TMP_FILE=tmp

echo Performing basic G.722_1 regression tests
echo

./broadvoice_tests E I 32000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_in.pcm $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_out_32000.itu
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests E failed!
    exit $RETVAL
fi
./broadvoice_tests E I 24000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_in.pcm $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_out_24000.itu
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests E failed!
    exit $RETVAL
fi
echo broadvoice_tests E completed OK

./broadvoice_tests D I 24000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_out_24000.itu $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_out_24000.pcm
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests D failed!
    exit $RETVAL
fi
./broadvoice_tests D I 32000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_enc_out_32000.itu $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_out_32000.pcm
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests D failed!
    exit $RETVAL
fi

./broadvoice_tests D I 24000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_in_24000_fe.itu $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_out_24000_fe.pcm
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests D failed!
    exit $RETVAL
fi
./broadvoice_tests D I 32000 16000 ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_in_32000_fe.itu $TMP_FILE
diff $TMP_FILE ../test-data/itu/g722_1/$VECTOR_CLASS/g722_1_dec_out_32000_fe.pcm
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests D failed!
    exit $RETVAL
fi
echo broadvoice_tests D completed OK

./broadvoice_tests E I 32000 16000 ../test-data/local/short_wb_voice.wav $TMP_FILE
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests E failed!
    exit $RETVAL
fi
echo broadvoice_tests E completed OK

./broadvoice_tests D I 32000 16000 $TMP_FILE test.au
RETVAL=$?
if [ $RETVAL != 0 ]
then
    echo broadvoice_tests D failed!
    exit $RETVAL
fi
echo broadvoice_tests D completed OK

echo
echo All regression tests successfully completed
